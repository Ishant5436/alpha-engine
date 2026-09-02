#!/usr/bin/env python3
"""
Black-Box Test Suite for AlphaEngine
Executes compiled C++ binaries from the outside, verifying CLI arguments,
input boundary files, metrics schema generation, and stdin stream processing.
"""

import os
import struct
import subprocess
import tempfile
import json
import pytest
from pathlib import Path

ALPHA_ENGINE_ROOT = str(Path(__file__).resolve().parent.parent)
BIN_ENGINE = os.path.join(ALPHA_ENGINE_ROOT, "bin/alpha_engine")
BIN_LIVE = os.path.join(ALPHA_ENGINE_ROOT, "bin/live_trader")


def generate_binary_tick_file(filepath: str, count: int, base_price: float = 50000.0):
    """Generate deterministic binary tick file (56 bytes per struct Tick)."""
    # Struct Tick layout in types.hpp:
    # uint64_t timestamp_ns (8B)
    # double bid_price (8B)
    # double ask_price (8B)
    # double bid_size (8B)
    # double ask_size (8B)
    # double last_price (8B)
    # double volume (8B)
    # Total = 56 bytes
    with open(filepath, "wb") as f:
        price = base_price
        for i in range(count):
            ts = i * 1_000_000  # 1ms intervals
            delta = 2.0 if (i % 20 < 10) else -1.5
            price += delta
            bid = price - 0.50
            ask = price + 0.50
            bid_sz = 1.0
            ask_sz = 1.0
            last_p = price
            vol = 0.5
            packed = struct.pack("<Qdddddd", ts, bid, ask, bid_sz, ask_sz, last_p, vol)
            f.write(packed)


@pytest.fixture(scope="module")
def ensure_binaries_built():
    """Ensure bin/alpha_engine and bin/live_trader are compiled."""
    subprocess.run(["make", "all"], cwd=ALPHA_ENGINE_ROOT, check=True)


# ==============================================================================
# 1. Binary Backtester CLI Contract & Output Verification (Black-Box)
# ==============================================================================

def test_blackbox_alpha_engine_nonexistent_file(ensure_binaries_built):
    """Black-box: passing a nonexistent file must fail with non-zero exit code."""
    res = subprocess.run([BIN_ENGINE, "/tmp/nonexistent_tick_file_123.bin"], capture_output=True, text=True)
    assert res.returncode != 0
    assert "Failed to open tick data file" in res.stderr or "Failed to open" in res.stdout


def test_blackbox_alpha_engine_zero_byte_file(ensure_binaries_built):
    """Black-box: empty 0-byte file must be handled safely without crashing."""
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as tmp:
        tmp_path = tmp.name
    try:
        res = subprocess.run([BIN_ENGINE, tmp_path], capture_output=True, text=True)
        assert res.returncode == 0
        assert "Loaded 0 market ticks" in res.stdout
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)


def test_blackbox_alpha_engine_valid_dataset_and_metrics_schema(ensure_binaries_built):
    """Black-box: running backtester produces performance report and valid metrics.json."""
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as tmp:
        tmp_path = tmp.name
    try:
        generate_binary_tick_file(tmp_path, count=5000, base_price=60000.0)
        res = subprocess.run([BIN_ENGINE, tmp_path], cwd=ALPHA_ENGINE_ROOT, capture_output=True, text=True)
        assert res.returncode == 0
        assert "STRATEGY PERFORMANCE REPORT" in res.stdout
        assert "Throughput (Ticks/sec)" in res.stdout

        # Verify metrics.json
        metrics_file = os.path.join(ALPHA_ENGINE_ROOT, "metrics.json")
        assert os.path.exists(metrics_file)
        with open(metrics_file) as f:
            data = json.load(f)
        assert data["processed_ticks"] == 5000
        assert data["ticks_per_second"] > 100_000
        assert data["taker_fee_bps"] == 4.0
        assert "total_return_pct" in data
        assert "annualized_sharpe" in data
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)


# ==============================================================================
# 2. Live Trader Stdin Stream Fuzzing & Execution (Black-Box)
# ==============================================================================

def test_blackbox_live_trader_stdin_stream(ensure_binaries_built):
    """Black-box: feed synthetic CSV stream to live trader via stdin pipe."""
    # Stream format expected by live_trader:
    # ts_ms price qty side
    input_lines = [
        f"{i*1000} {50000.0+i} 1.0 BUY\n"
        for i in range(1, 15)
    ]
    proc = subprocess.Popen(
        [BIN_LIVE, "BTCUSDT", "10000.0"],
        cwd=ALPHA_ENGINE_ROOT,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    stdout, stderr = proc.communicate(input="".join(input_lines), timeout=5)
    assert proc.returncode == 0
    assert "ALPHAENGINE LIVE WEBSOCKET FORWARD PAPER TRADER" in stdout
    assert "BTCUSDT" in stdout


def test_blackbox_live_trader_fuzz_garbage_lines(ensure_binaries_built):
    """Black-box: feed malformed / garbage ASCII lines through stdin without crashing."""
    garbage_input = [
        "GARBAGE_DATA_CORRUPT\n",
        "\n",
        ",,,,\n",
        "invalid,not_a_number,abc,xyz\n",
        "99999999999999999999999999999999999999999999\n",
    ]
    proc = subprocess.Popen(
        [BIN_LIVE, "ETHUSDT", "5000.0"],
        cwd=ALPHA_ENGINE_ROOT,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    stdout, stderr = proc.communicate(input="".join(garbage_input), timeout=5)
    assert proc.returncode == 0
