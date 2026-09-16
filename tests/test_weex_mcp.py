"""
Unit Tests for WEEX AI Wars Trader Skill FastMCP Server
Validates tool definitions, deterministic responses, Power of 10 invariants,
and dry-run execution safety.
"""
import json
import pytest
from scripts.weex_mcp_server import (
    weex_stability_shield_status,
    weex_risk_gated_order,
    weex_cancel_order,
    weex_ticker,
    weex_orderbook,
    weex_account_balance,
    _SHIELD
)


def test_weex_mcp_stability_shield_status():
    """Verify stability shield status reports compliant rule 127 metrics."""
    res_str = weex_stability_shield_status()
    data = json.loads(res_str)
    assert data["status"] == "success"
    assert data["rule_127_compliant"] is True
    assert data["circuit_breaker_active"] is False
    assert data["shield"]["starting_equity"] == 10000.0
    assert data["shield"]["max_daily_drawdown_limit_pct"] == 2.0


def test_weex_mcp_risk_gated_order_simulation():
    """Verify dry-run risk-gated limit order placement."""
    res_str = weex_risk_gated_order(
        symbol="cmt_btcusdt",
        side="BUY",
        pos_side="LONG",
        qty=0.05,
        price=59500.00,
        dry_run=True
    )
    data = json.loads(res_str)
    assert data["status"] == "success"
    assert data["dry_run"] is True
    assert data["response"]["status"] == "DRY_RUN_SIMULATED"


def test_weex_mcp_circuit_breaker_blocks_order():
    """Verify orders are rejected when circuit breaker trips."""
    _SHIELD.update_equity(9700.0)  # 3% drawdown > 2% limit
    assert _SHIELD.is_tripped is True

    res_str = weex_risk_gated_order(
        symbol="cmt_btcusdt",
        side="BUY",
        pos_side="LONG",
        qty=0.05,
        price=59500.00,
        dry_run=True
    )
    data = json.loads(res_str)
    assert data["status"] == "rejected"
    assert data["reason"] == "CIRCUIT_BREAKER_TRIPPED"

    # Reset daily
    _SHIELD.reset_daily(10000.0)
    assert _SHIELD.is_tripped is False


def test_weex_mcp_cancel_order():
    """Verify dry-run cancel order."""
    res_str = weex_cancel_order(symbol="cmt_btcusdt", order_id="ORDER-999", dry_run=True)
    data = json.loads(res_str)
    assert data["status"] == "DRY_RUN_CANCELLED"
    assert data["order_id"] == "ORDER-999"


def test_weex_mcp_ticker_and_orderbook():
    """Verify ticker and orderbook tools return structured JSON."""
    ticker_data = json.loads(weex_ticker("cmt_btcusdt"))
    assert ticker_data["symbol"] == "cmt_btcusdt"
    assert "ticker" in ticker_data

    book_data = json.loads(weex_orderbook("cmt_btcusdt", depth=10))
    assert book_data["symbol"] == "cmt_btcusdt"
    assert "spread_bps" in book_data
    assert book_data["bids_count"] > 0
