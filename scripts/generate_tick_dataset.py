#!/usr/bin/env python3
"""
Synthetic High-Frequency Crypto Tick Dataset Generator
Simulates realistic regime-switching micro-structure (Momentum trends + Mean-reverting ranges + Volatility shocks).
"""

import numpy as np
import struct
import sys
import os

def generate_ticks(filename: str, num_ticks: int = 500000):
    np.random.seed(42)
    print(f"Generating {num_ticks:,} high-frequency ticks -> {filename}...")

    # Regime-switching Markov chain
    # 0 = Range (mean-reverting around anchor), 1 = Bull trend, 2 = Bear trend
    regimes = np.zeros(num_ticks, dtype=int)
    regime = 0
    transitions = [
        [0.995, 0.003, 0.002],
        [0.004, 0.994, 0.002],
        [0.004, 0.002, 0.994]
    ]

    for i in range(1, num_ticks):
        regime = np.random.choice([0, 1, 2], p=transitions[regime])
        regimes[i] = regime

    prices = np.zeros(num_ticks, dtype=float)
    prices[0] = 50000.0
    anchor = 50000.0

    for i in range(1, num_ticks):
        r = regimes[i]
        prev = prices[i-1]
        if r == 0:
            # Mean reversion towards current anchor with noise
            drift = -0.01 * (prev - anchor)
            vol = 0.8
        elif r == 1:
            # Bullish trend
            drift = 0.25
            vol = 1.0
            anchor = prev
        else:
            # Bearish trend
            drift = -0.25
            vol = 1.0
            anchor = prev

        shock = np.random.normal(drift, vol)
        prices[i] = max(100.0, prev + shock)

    # Write binary format
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "wb") as f:
        for i in range(num_ticks):
            ts = i * 1000000 # 1ms
            p = prices[i]
            spread = 0.10 + np.random.uniform(0.02, 0.08)
            bid = p - spread / 2.0
            ask = p + spread / 2.0
            vol = np.random.uniform(0.5, 3.0)
            f.write(struct.pack("Qdddddd", ts, bid, ask, 10.0, 10.0, p, vol))

    print(f"[SUCCESS]  Generated {num_ticks:,} ticks in binary format.")

if __name__ == "__main__":
    out_file = sys.argv[1] if len(sys.argv) > 1 else "/Users/ishantpanchal/alpha-engine/data/ticks_500k.bin"
    generate_ticks(out_file, 500000)
