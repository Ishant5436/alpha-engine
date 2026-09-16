"""
Tests for WEEX AI Wars II Live Execution Gateway (alpha-engine)
TDD Verification Suite:
- Signature generation (HMAC-SHA256 Base64)
- Trade tick stream normalization for C++ live_trader
- Token bucket rate limiting (<= 40 requests/min)
- Order payload construction with positionSide
- Dry-run execution safety invariant
- Zero credential leakage
"""
import asyncio
import base64
import hashlib
import hmac
import json

from scripts.weex_gateway import (
    WeexSigner,
    TokenBucketLimiter,
    normalize_weex_trade,
    build_weex_order_payload,
    WeexRestClient,
    CompetitionStabilityShield,
)


def test_weex_signature_vector():
    """Verify HMAC-SHA256 Base64 signature calculation against mathematical standard."""
    secret = "test_weex_secret_key_12345"
    signer = WeexSigner(
        api_key="test_api_key",
        secret_key=secret,
        passphrase="test_passphrase"
    )
    timestamp = "1726417200000"
    method = "POST"
    request_path = "/capi/v3/order"
    body = '{"symbol":"BTCUSDT","side":"BUY","quantity":"0.01"}'

    sign = signer.generate_signature(
        timestamp=timestamp,
        method=method,
        request_path=request_path,
        body=body
    )

    # Recompute ground-truth signature
    payload = f"{timestamp}{method.upper()}{request_path}{body}"
    expected_hmac = hmac.new(
        secret.encode("utf-8"),
        payload.encode("utf-8"),
        hashlib.sha256
    ).digest()
    expected_sign = base64.b64encode(expected_hmac).decode("utf-8")

    assert sign == expected_sign
    assert len(sign) > 0


def test_weex_trade_normalization_flat():
    """Parse standard flat WEEX trade message into space-delimited tick format."""
    raw_msg = json.dumps({
        "e": "trade",
        "E": 1726417200100,
        "s": "BTCUSDT",
        "p": "60250.25",
        "q": "0.1250",
        "T": 1726417200000,
        "m": False  # buyer is taker -> BUY
    })

    tick_line = normalize_weex_trade(raw_msg)
    assert tick_line == "1726417200000 60250.25 0.1250 BUY"


def test_weex_trade_normalization_nested():
    """Parse stream-wrapped WEEX trade frame into space-delimited tick format."""
    raw_msg = json.dumps({
        "stream": "BTCUSDT@trade",
        "data": {
            "p": "60255.00",
            "q": "0.0500",
            "T": 1726417201000,
            "m": True  # buyer is maker -> seller is taker -> SELL
        }
    })

    tick_line = normalize_weex_trade(raw_msg)
    assert tick_line == "1726417201000 60255.00 0.0500 SELL"


def test_weex_trade_normalization_invalid_returns_none():
    """Malformed or ping messages must be safely ignored without exceptions."""
    assert normalize_weex_trade("PONG") is None
    assert normalize_weex_trade(json.dumps({"method": "PONG"})) is None
    assert normalize_weex_trade("not json") is None
    assert normalize_weex_trade(json.dumps({"p": "-10.0", "q": "0.1"})) is None


def test_token_bucket_rate_limiter():
    """Ensure token bucket rate limiter strictly bounds burst requests."""
    async def _test():
        # 5 requests per second limit for test speed
        limiter = TokenBucketLimiter(capacity=5, refill_rate_per_sec=5.0)

        # 5 requests should acquire immediately
        for _ in range(5):
            assert limiter.try_acquire() is True

        # 6th immediate request must be rejected / throttled
        assert limiter.try_acquire() is False

        # After 250ms, some tokens replenish
        await asyncio.sleep(0.25)
        assert limiter.try_acquire() is True

    asyncio.run(_test())


def test_build_weex_order_payload():
    """Verify conversion from C++ engine ORDER signal into WEEX V3 contract payload."""
    # Format: ORDER <symbol> <side> <positionSide> <qty> <price> <confidence>
    signal_line = "ORDER BTCUSDT BUY LONG 0.050000 60120.50 1.0000"

    payload = build_weex_order_payload(signal_line)
    assert payload["symbol"] == "BTCUSDT"
    assert payload["side"] == "BUY"
    assert payload["positionSide"] == "LONG"
    assert payload["type"] == "LIMIT"
    assert payload["timeInForce"] == "GTC"
    assert float(payload["quantity"]) == 0.05
    assert float(payload["price"]) == 60120.50
    assert payload["newClientOrderId"].startswith("ALPHA-")


def test_credential_masking():
    """Ensure API secret and passphrase are redacted in repr and string outputs."""
    signer = WeexSigner(
        api_key="weex_live_key_9999",
        secret_key="super_secret_private_key_xyz",
        passphrase="my_secret_passphrase"
    )
    s_repr = repr(signer)
    s_str = str(signer)

    assert "super_secret_private_key_xyz" not in s_repr
    assert "my_secret_passphrase" not in s_repr
    assert "super_secret_private_key_xyz" not in s_str
    assert "my_secret_passphrase" not in s_str
    assert "***" in s_repr or "REDACTED" in s_repr


def test_dry_run_safety_invariant():
    """Ensure dry-run mode returns simulated success without executing network calls."""
    async def _test():
        signer = WeexSigner("k", "s", "p")
        client = WeexRestClient(signer=signer, dry_run=True)

        order_payload = {
            "symbol": "BTCUSDT",
            "side": "BUY",
            "type": "LIMIT",
            "quantity": "0.01",
            "price": "60000.00",
            "positionSide": "LONG"
        }

        result = await client.place_order(order_payload)
        assert result["status"] == "DRY_RUN_SIMULATED"
        assert result["orderId"].startswith("SIM-")
        assert result["symbol"] == "BTCUSDT"

    asyncio.run(_test())


def test_stability_shield_initialization_and_invariants():
    """Verify input validation and default state for CompetitionStabilityShield."""
    shield = CompetitionStabilityShield(starting_equity=10000.0, max_daily_drawdown_pct=0.02, min_flat_pct=0.80)
    assert shield.starting_equity == 10000.0
    assert shield.current_equity == 10000.0
    assert shield.max_daily_drawdown_pct == 0.02
    assert not shield.is_tripped
    assert shield.can_open_order(is_closing=False)
    assert shield.flat_ratio == 1.0


def test_stability_shield_drawdown_trips_circuit_breaker():
    """Verify circuit breaker trips at 2% drawdown and blocks entries but allows exits."""
    shield = CompetitionStabilityShield(starting_equity=10000.0, max_daily_drawdown_pct=0.02)

    # 1.0% drop: starting 10,000 -> 9,900 (drawdown 1.0% < 2.0%)
    intact = shield.update_equity(9900.0)
    assert intact
    assert not shield.is_tripped
    assert shield.can_open_order(is_closing=False)

    # 2.0% drop: 10,000 -> 9,800 (drawdown 2.0% >= 2.0% threshold)
    intact = shield.update_equity(9800.0)
    assert not intact
    assert shield.is_tripped
    # New entries blocked
    assert not shield.can_open_order(is_closing=False)
    # Closing exits must ALWAYS be permitted
    assert shield.can_open_order(is_closing=True)


def test_stability_shield_daily_reset():
    """Verify circuit breaker reset restores order entry permissions."""
    shield = CompetitionStabilityShield(starting_equity=10000.0, max_daily_drawdown_pct=0.02)
    shield.update_equity(9700.0)  # 3% drawdown -> trips
    assert shield.is_tripped

    # Daily rollover at 00:00 UTC resets equity baseline
    shield.reset_daily(new_starting_equity=9700.0)
    assert not shield.is_tripped
    assert shield.starting_equity == 9700.0
    assert shield.can_open_order(is_closing=False)


def test_stability_shield_flat_residency_tracking():
    """Verify tracking of time spent in FLAT state to prevent retail taker fee bleed."""
    shield = CompetitionStabilityShield(starting_equity=10000.0)

    # 8 ticks in FLAT state (position_size = 0.0)
    for _ in range(8):
        shield.update_tick(price=60000.0, position_size=0.0)

    # 2 ticks in active position (position_size = 0.5)
    for _ in range(2):
        shield.update_tick(price=60100.0, position_size=0.5)

    assert shield.total_ticks == 10
    assert shield.flat_ticks == 8
    assert shield.flat_ratio == 0.80  # Exactly 80% FLAT state


def test_rest_client_circuit_breaker_blocks_entry():
    """Verify WeexRestClient blocks entry orders when shield is tripped, but allows exits."""
    async def _test():
        shield = CompetitionStabilityShield(starting_equity=10000.0, max_daily_drawdown_pct=0.02)
        shield.update_equity(9750.0)  # 2.5% drawdown -> tripped
        assert shield.is_tripped

        signer = WeexSigner("k", "s", "p")
        client = WeexRestClient(signer=signer, dry_run=True, stability_shield=shield)

        # Entry order: BUY to open LONG -> must be BLOCKED
        entry_payload = {
            "symbol": "BTCUSDT",
            "side": "BUY",
            "type": "LIMIT",
            "quantity": "0.01",
            "price": "60000.00",
            "positionSide": "LONG"
        }
        res_entry = await client.place_order(entry_payload)
        assert res_entry["error"] is True
        assert res_entry["status"] == "CIRCUIT_BREAKER_BLOCKED"

        # Exit order: SELL to close LONG -> must be ALLOWED
        exit_payload = {
            "symbol": "BTCUSDT",
            "side": "SELL",
            "type": "LIMIT",
            "quantity": "0.01",
            "price": "60500.00",
            "positionSide": "LONG"
        }
        res_exit = await client.place_order(exit_payload)
        assert res_exit["status"] == "DRY_RUN_SIMULATED"
        assert res_exit["orderId"].startswith("SIM-")

    asyncio.run(_test())

