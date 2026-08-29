#!/usr/bin/env python3
"""
Synthetic High-Frequency Crypto Tick Dataset Generator
Simulates realistic regime-switching micro-structure (Momentum trends + Mean-reverting ranges + Volatility shocks).
"""

import numpy as np
import struct
import sys

def generate_ticks(filename: str, num_ticks: int = 1000000):
    np.random.seed(42)
    print(f"Generating {num_ticks:,} high-frequency ticks -> {filename}...")

    # Regime-switching Markov chain
    # Regime 0: Mean-reverting range
    # Regime 1: Bullish momentum breakout
    # Regime 2: Volatility shock / Bearish trend
    regimes = np.zeros(num_ticks, dtype=int)
    regime = 0
    transitions = [
        [0.998, 0.0015, 0.0005],
        [0.002, 0.997,  0.001],
        [0.005, 0.002,  0.993]
    ]

    for i in range(1, num_ticks):
        regime = np.random.choice([0, 1, 2], p=transitions[regime])
        regimes[i] = regime

    prices = np.zeros(num_ticks, dtype=float)
    prices[0] = 50000.0 # Initial BTC/USDT price

    for i in range(1, num_ticks):
        r = regimes[i]
        if r == 0:
            drift = -0.0005 * (prices[i-1] - 50000.0) # Mean-reversion
            vol = 0.5
        elif r == 1:
            drift = 0.08 # Upward momentum
            vol = 1.2
        else:
            drift = -0.06 # Downward trend
            vol = 2.0

        shock = np.random.normal(drift, vol)
        prices[i] = max(100.0, prices[i-1] + shock)

    # Write binary format: struct Tick (uint64 timestamp, double bid, ask, bid_sz, ask_sz, last, vol)
    with open(filename, "wb") as f:
        for i in range(num_ticks):
            ts = i * 1000000 # 1ms steps
            p = prices[i]
            spread = 0.5 + np.random.uniform(0.1, 0.5)
            bid = p - spread / 2.0
            ask = p + spread / 2.0
            vol = np.random.uniform(0.1, 5.0)
            f.write(struct.pack("Qdddddd", ts, bid, ask, 10.0, 10.0, p, vol))

    print(f"✅ Generated {num_ticks:,} ticks in binary format.")

if __name__ == "__main__":
    out_file = sys.argv[1] if len(sys.argv) > 1 else "/Users/ishantpanchal/alpha-engine/data/ticks_1m.bin"
    import os
    os.makedirs(os.path.dirname(out_file), exist_ok=True)
    generate_ticks(out_file, 500000)
