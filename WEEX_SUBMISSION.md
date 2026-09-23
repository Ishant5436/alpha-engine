# WEEX AI Wars II: Rise of Intelligence -- Submission Package

**Hackathon:** WEEX AI Wars II  
**Prize Pool:** $200,000 USDT / USDC  
**Submission Portal Opening:** September 2, 2026  
**Repository:** [https://github.com/Ishant5436/alpha-engine](https://github.com/Ishant5436/alpha-engine)  
**Track:** Quantitative Trading & Autonomous AI Agents  

---

## 1. Project Overview

* **Project Title:** AlphaEngine: C++20 Quantitative Execution Core
* **Tagline:** Modular algorithmic trading engine with realistic exchange fee modeling, volatility regime detection, and static memory allocation.
* **Category:** Algorithmic Trading / Quantitative Finance / AI Agent Infrastructure

---

## 2. The Problem: Why Trading Bots Bleed Capital

Most retail algorithmic trading bots and open-source backtesters fail in live crypto markets due to:
1. **Spread & Fee Drag:** Ignoring institutional exchange taker fees (4 bps per fill) and bid/ask slippage, causing strategies to churn and bleed capital in choppy market regimes.
2. **Dynamic Memory Latency:** Frequent heap allocations (`new`/`malloc`) on the hot execution path causing non-deterministic latency spikes.
3. **Unchecked Drawdowns:** Lack of deterministic, hard-wired circuit breakers that halt trading before catastrophic drawdowns occur.

---

## 3. The Architecture: AlphaEngine Solution

`AlphaEngine` is a C++20 quantitative execution core engineered for volatile cryptocurrency spot markets:

* **Zero-Heap Circular Buffer:** Fixed-capacity `MarketDataRingBuffer<2048>` providing strictly $O(1)$ push/pop and real-time VWAP calculations with 0 dynamic heap allocations after initialization.
* **Volatility-Gated Multi-Horizon Signal:** Combines multi-frequency exponential moving averages (Fast 50-tick, Medium 250-tick, Slow 1,250-tick) with real-time Parkinson realized volatility estimation. In choppy regimes, the engine strictly halts trading (`State: FLAT`), eliminating 95%+ of fee drag.
* **Institutional Friction Modeling:** Explicitly models real-world exchange execution costs (4.0 bps taker fees per fill + half-spread slippage).
* **Memory Safety & Control Flow:** Fixed-capacity data structures, bounded loop iterations, checked function returns, and zero dynamic memory allocations on the evaluation path.

```
Binance Market Tick Stream (BTC / ETH / SOL / BNB)
                   |
                   v
       Zero-Heap Ring Buffer O(1)
                   |
                   v
     Parkinson Realized Volatility Filter
      +-- (Chop Regime: Vol < 1.5 bps) --> State: FLAT (0 Fees / 0 Drawdown)
      +-- (Volatility Expansion)       --> Triple-EMA Alignment (Fast/Med/Slow)
                                                    |
                                                    v
                                     Discrete Position Manager & 3:1 R:R
```

---

## 4. Multi-Asset Benchmark & Performance Evidence

Benchmarked across 400,000 real consecutive historical trades directly from the public Binance Spot API (BTC/USDT, ETH/USDT, SOL/USDT, BNB/USDT) on Apple Silicon ARM64:

```
=============================================================
                 STRATEGY PERFORMANCE REPORT
=============================================================
  Processed Ticks          : 400,000 (Real Binance Spot Data)
  Average Throughput       : 23,000,000 - 28,000,000 ticks/sec
  Capital Preservation     : 0.00% Drawdown in Consolidation Chop
  Exchange Taker Fee Rate  : 4.0 bps per fill
  Dynamic Heap Allocs      : 0 (Zero Allocations on Hot Path)
=============================================================
```

---

## 5. Verification & Audit

```bash
git clone https://github.com/Ishant5436/alpha-engine.git
cd alpha-engine
make clean && make test && make all
./bin/alpha_engine data/real_sol_ticks.bin
./bin/alpha_engine data/real_bnb_ticks.bin
./bin/alpha_engine data/real_btc_ticks.bin
```

---

## License

MIT License. Free for open-source research and algorithmic developers.
