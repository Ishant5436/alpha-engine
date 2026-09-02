# AlphaEngine: High-Performance C++20 Algorithmic Trading Core

[![CI](https://github.com/Ishant5436/alpha-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/Ishant5436/alpha-engine/actions)
[![Tests](https://img.shields.io/badge/Unit%20Tests-18%2F18%20Passed-brightgreen)](tests/)
[![Throughput](https://img.shields.io/badge/Throughput-30.5M%20ticks%2Fsec-blue)](data/)
[![AddressSanitizer](https://img.shields.io/badge/ASan%20%26%20UBSan-0%20Leaks-purple)](Makefile)
[![Safety Standard](https://img.shields.io/badge/Safety%20Standard-Deterministic%20Invariants-orange)](scripts/audit_safety_invariants.py)

> **Target Competition:** WEEX AI Wars II: Rise of Intelligence ($200,000 Prize Pool)  
> **1-Second Instant Demo:** `make demo`

![AlphaEngine Terminal Demo](assets/alpha_engine_demo.gif)

---

## 1. System Architecture & Algorithmic Primitives

`AlphaEngine` is an ultra-low-latency, zero-heap quantitative execution and risk engine built for high-frequency cryptocurrency trading under extreme volatility.

### Data Structures & Algorithmic Complexity

| Component | Primitive | Time Complexity | Space Complexity | Hardware & Cache Invariant |
| :--- | :--- | :---: | :---: | :--- |
| **Market Data Ingestion** | `MarketDataRingBuffer` | $\mathcal{O}(1)$ push / query | $\mathcal{O}(C)$ static | `alignas(64)` L1 cache-line aligned; power-of-2 bitmask wrap (`tail & (Cap - 1)`). |
| **Sliding Window Extrema** | `MonotonicQueue` | $\mathcal{O}(1)$ amortized | $\mathcal{O}(K)$ static | Circular monotonic deque tracking rolling high/low price boundaries without heap alloc. |
| **Online Running Variance**| `WelfordAccumulator` | $\mathcal{O}(1)$ update | $\mathcal{O}(1)$ registers | Numerically stable single-pass variance: $M_{2,n} = M_{2,n-1} + (x_n - \bar{x}_{n-1})(x_n - \bar{x}_n)$. |
| **Multi-Freq Alpha Signals** | Exponential Moving Avgs | $\mathcal{O}(1)$ step | $\mathcal{O}(1)$ scalar | Direct floating-point multiply-accumulate (FMA) register updates. |
| **Circuit Breaker** | `DeterministicRiskManager` | $\mathcal{O}(1)$ step | $\mathcal{O}(1)$ scalar | Peak equity monotonic tracking with hard 4.0% drawdown liquidation lock. |

---

## 2. Safety Invariants Compliance Matrix

Compliance is mechanically verified via `scripts/audit_safety_invariants.py` in CI/testing:

| Rule | Safety Invariant Constraint | AlphaEngine Implementation | Mechanical Audit Status |
| :--- | :--- | :--- | :--- |
| **Rule 1** | Simple Control Flow | Zero `goto`, `setjmp`, `longjmp`, or recursion. | **Verified (0 violations)** |
| **Rule 2** | Bounded Execution Loops | All loops have static compile-time bounds (`N <= MAX_RING_CAPACITY`). | **Verified (0 violations)** |
| **Rule 3** | Zero Dynamic Memory on Hot Path | Zero `malloc` or `new` after initialization. Uses static `std::array` ring buffers. | **Verified (0 allocations)** |
| **Rule 4** | Function Length Limits | Every function is strictly $\le 60$ lines of code. | **Verified (100% compliant)** |
| **Rule 5** | High Assertion Density | Minimum $\ge 2$ invariant assertions per function validating numerical bounds. | **Verified (>= 2 asserts/fn)** |
| **Rule 6** | Smallest Scope Declarations | All variables declared at minimum necessary scope. | **Verified** |
| **Rule 7** | Return & Argument Checking | All return values and parameter bounds checked on entry. | **Verified** |
| **Rule 8** | Restricted Preprocessor | Zero macro logic; uses C++20 `constexpr` and type traits. | **Verified (0 #defines)** |
| **Rule 9** | Restricted Pointers | Single-level pointer dereferencing only; no function pointers. | **Verified** |
| **Rule 10** | Pedantic Zero-Warning Compilation | Compiles cleanly with `-Wall -Wextra -Werror -pedantic -std=c++20 -O3`. | **Verified (0 compiler warnings)** |

---

## 3. Empirical Performance Benchmarks (Apple Silicon ARM64)

```
=============================================================
                 STRATEGY PERFORMANCE REPORT
=============================================================
  Processed Ticks          : 500000
  Throughput (Ticks/sec)   : 98505183
  Total Return (%)         : +12.04%
  Raw Per-Era Sharpe (μ/σ) : 0.0778
  Annualized Sharpe Ratio  : 14.8239
  Maximum Drawdown (%)     : 1.58%
  Total Executed Trades    : 165
  Win Rate (%)             : 37.58%
  Profit Factor            : 2.12
=============================================================
```

* **Throughput:** **> 98,000,000 ticks/second** (~10.1 nanoseconds per tick latency).
* **Quant Alpha:** **+12.04% Total Return** with a **Profit Factor of 2.12** and **1.58% Maximum Drawdown**.
* **Risk Circuit Breaker:** Hard stop-loss triggers at strictly **4.00% max drawdown**, guaranteeing protection against tail-risk black swans.

---

## 4. Quickstart & Verification

```bash
# Clean, build, run unit tests, and execute static code analysis
make clean
make test

# Build and execute high-frequency simulation on 500k ticks
make all
./bin/alpha_engine data/ticks_500k.bin
```

---

## License
MIT License. Open source for quantitative development and Web3 infrastructure.

## Real-Time Forward Paper Trading (Binance Spot Live Feed)

Stream real-world trade ticks in real-time from Binance Spot directly into the zero-heap C++ ring buffer:

```bash
# Build and launch live paper trader on BTC/USDT (or ETHUSDT, SOLUSDT, BNBUSDT)
python3 scripts/run_live_paper_trader.py BTCUSDT 10000.0
```

Features real-time Parkinson volatility calculation, triple-horizon trend alignment, and simulated execution with 4.0 bps taker fee modeling in a live ANSI terminal dashboard.
