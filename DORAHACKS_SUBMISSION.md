# WEEX AI Wars II — DoraHacks Submission Package

**Hackathon:** WEEX AI Wars II: Rise of Intelligence  
**Prize Pool:** $200,000 USDT / USDC  
**BUIDL Profile:** [#48230](https://dorahacks.io/buidl/48230)  
**Submission ID:** 54207  
**Track:** Quantitative Trading & Autonomous AI Agents  
**Author:** Ishant Panchal (`Ishant5436` / `ishant.p@somaiya.edu`)  
**Repository:** [https://github.com/Ishant5436/alpha-engine](https://github.com/Ishant5436/alpha-engine)  

---

## 1. Project Overview

* **Project Title:** AlphaEngine: Zero-Heap C++20 High-Frequency Quantitative Execution Core
* **Tagline:** High-throughput, deterministic algorithmic trading engine processing >98,000,000 ticks/sec with institutional friction modeling and Power of 10 safety invariants.
* **Category:** Algorithmic Trading / Quantitative Finance / AI Agent Infrastructure

---

## 2. Problem Statement
Most retail algorithmic trading bots and open-source backtesters fail in live crypto markets due to:
1. **Spread & Fee Oblivion:** Ignoring institutional exchange taker fees (4 bps per side) and bid/ask slippage, causing strategies to bleed capital in choppy market regimes.
2. **Dynamic Memory Latency:** Frequent heap allocations (`new`/`malloc`) on the hot execution path causing non-deterministic garbage collection / heap fragmentation latency spikes.
3. **Unchecked Drawdowns:** Lack of deterministic, hard-wired circuit breakers that halt trading before catastrophic drawdowns occur.

---

## 3. The Solution: AlphaEngine Architecture
`AlphaEngine` is an ultra-low-latency, zero-allocation C++20 quantitative trading engine engineered for high-frequency crypto asset markets:

* **Zero-Heap Circular Buffer:** Fixed-capacity `MarketDataRingBuffer<2048>` providing strictly $O(1)$ push/pop and real-time VWAP calculations with 0 dynamic heap allocations after initialization.
* **Volatility-Gated Multi-Horizon Signal:** Combines multi-frequency exponential moving averages (Fast 50-tick, Medium 250-tick, Slow 1,250-tick) with real-time Parkinson realized volatility estimation. In choppy/low-volatility regimes, the engine strictly halts trading (`State: FLAT`), eliminating 95%+ of fee drag.
* **Institutional Friction Modeling:** Explicitly models real-world exchange execution costs (4.0 bps taker fees per fill + half-spread slippage).
* **Deterministic Safety Invariants:** Adheres strictly to Deterministic Safety Invariants (Power of 10 Rules: functions $\le 60$ lines, assertion density $\ge 2$, bounded loops, zero preprocessor macros).

```mermaid
graph TD
    A[Real Binance Market Tick Stream] --> B[Zero-Heap Ring Buffer O(1)]
    B --> C[Parkinson Realized Volatility Filter]
    C -->|Chop Regime: Vol < 1.5 bps| D[State: FLAT -> 0 Fees / 0 Drawdown]
    C -->|Volatility Expansion > 1.5 bps| E[Triple-EMA Multi-Horizon Alignment]
    E --> F[Discrete Position Manager]
    F --> G[Asymmetric 3:1 Execution & 4% Hard Circuit Breaker]
```

---

## 4. Benchmark & Performance Evidence
Benchmarked on 500,000 real consecutive historical trades directly from the public Binance Spot API (BTC/USDT & ETH/USDT) on Apple Silicon ARM64:

```
=============================================================
                 STRATEGY PERFORMANCE REPORT
=============================================================
  Processed Ticks          : 500,000 (Real Binance Spot)
  Throughput (Ticks/sec)   : 98,505,183 ticks/sec
  Total Return (%)         : +12.04%
  Raw Per-Era Sharpe (μ/σ) : 0.0778
  Maximum Drawdown (%)     : 1.82% (4.00% Hard Circuit Breaker)
  Exchange Taker Fee Rate  : 4.0 bps per fill
  Dynamic Heap Allocs      : 0 (Zero Allocations on Hot Path)
  Test Suite               : 18/18 C++ tests under ASan/UBSan (100% Passed)
=============================================================
```

---

## 5. Visual Walkthrough & Demo Assets

* **Interactive Terminal Demo:** [`assets/alpha_engine_demo.gif`](https://raw.githubusercontent.com/Ishant5436/alpha-engine/main/assets/alpha_engine_demo.gif)
* **High-Definition Video:** [`assets/alpha_engine_demo.mp4`](https://github.com/Ishant5436/alpha-engine/raw/main/assets/alpha_engine_demo.mp4)

---

## 6. Quick Verification & Demo Commands
Reviewers and judges can clone, audit, compile, and run the engine locally in under 10 seconds:

```bash
# 1. Clone Repository
git clone https://github.com/Ishant5436/alpha-engine.git
cd alpha-engine

# 2. Run Comprehensive Test Suite (18/18 Tests Passing under ASan/UBSan)
make test

# 3. Execute Interactive Terminal Demo Walkthrough
make demo
```
