# AlphaEngine: High-Performance C++20 Algorithmic Trading Core

**Target Competition:** WEEX AI Wars II: Rise of Intelligence ($200,000 Prize Pool)  
**Author:** Ishant Panchal (`Ishant5436`)  
**Core Language:** C++20 (Zero-Heap Hot Path, Optimized for Apple Silicon ARM64)  
**Safety Standard:** Deterministic Safety-Critical Systems Invariants  

---

## 1. System Architecture

`AlphaEngine` is an ultra-low-latency, zero-heap quantitative execution and risk engine built for high-frequency cryptocurrency trading under extreme volatility.

```mermaid
graph TD
    A[L2/Tick Stream Ingestion] --> B[Fixed-Capacity Ring Buffer (Zero Heap)]
    B --> C[Multi-Frequency Alpha Engine (Fast/Slow EMA Spread)]
    C -->|Momentum Breakout| D[Target Position Engine]
    C -->|Mean Reversion Range| E[VWAP Signal Filter]
    D --> F[Deterministic Risk Guard]
    E --> F
    F -->|Drawdown >= 4.0%| G[Deterministic Circuit Breaker / Liquidation]
    F -->|Approved Orders| H[Sub-Microsecond Execution Fill]
    H --> I[Sharpe & Telemetry Monitor]
```

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
