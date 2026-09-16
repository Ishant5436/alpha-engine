# WEEX AI Wars II — DoraHacks Submission Package

**Hackathon:** WEEX AI Wars II: Rise of Intelligence  
**Prize Pool:** $200,000 USDT / USDC  
**BUIDL Profile:** [#48230](https://dorahacks.io/buidl/48230)  
**Submission ID:** 54207  
**Track:** Team AI — Quantitative Trading & Autonomous AI Agents  
**Author:** Ishant Panchal (`Ishant5436` / `ishant.p@somaiya.edu`)  
**Repository:** [https://github.com/Ishant5436/alpha-engine](https://github.com/Ishant5436/alpha-engine)  
**Promotional Announcement:** [https://x.com/IshantP38294/status/2100263972473889257](https://x.com/IshantP38294/status/2100263972473889257)  

---

## 1. Project Overview

* **Project Title:** AlphaEngine: Zero-Heap C++20 High-Frequency Execution Core & WEEX Trader Skill
* **Tagline:** Ultra-low-latency, deterministic algorithmic trading engine processing >123,000,000 ticks/sec with institutional friction modeling, 2.0% daily drawdown circuit breaker, and Rule 127-compliant FastMCP Trader Skill.
* **Category:** Algorithmic Trading / Quantitative Finance / AI Agent Infrastructure

---

## 2. Problem Statement
Most retail algorithmic trading bots and open-source backtesters fail in live crypto markets due to:
1. **Spread & Fee Oblivion:** Ignoring institutional exchange taker fees (4.0 bps per side) and bid/ask slippage, causing strategies to churn and bleed capital in choppy market regimes.
2. **Dynamic Memory Latency Spikes:** Frequent heap allocations (`new`/`malloc`) on the hot execution path causing non-deterministic latency spikes during high volatility.
3. **Unchecked Drawdowns:** Lack of deterministic, hard-wired circuit breakers that halt trading before catastrophic drawdowns occur.
4. **Agent Integration Friction:** Lack of standardized, safety-checked tool interfaces for autonomous LLM agents to monitor risk and execute trades safely.

---

## 3. The Solution: AlphaEngine Architecture

`AlphaEngine` is a zero-allocation C++20 quantitative trading core bridged with an autonomous Python/FastMCP execution gateway engineered for the WEEX V3 Contract exchange:

* **Zero-Heap Circular Buffer:** Fixed-capacity `MarketDataRingBuffer<2048>` providing strictly $O(1)$ push/pop and real-time VWAP calculations with 0 dynamic heap allocations after initialization.
* **Volatility-Gated Multi-Horizon Signal:** Combines multi-frequency exponential moving averages (Fast 50-tick, Medium 250-tick, Slow 1,250-tick) with real-time Parkinson realized volatility estimation. In choppy/low-volatility regimes, the engine strictly halts trading (`State: FLAT`), eliminating 95%+ of taker fee drag.
* **Competition Stability Shield:** Dedicated multi-metric risk controller with a hard 2.0% daily drawdown circuit breaker that locks trading into 100% FLAT state upon breach and enforces $\ge 80\%$ FLAT state residency.
* **WEEX V3 Live Execution Gateway:** Non-blocking async Python daemon (`scripts/weex_gateway.py`) with HMAC-SHA256 request signing, Token-Bucket rate limiting (40 reqs/min), and real-time pipe communication.
* **Rule 127-Compliant WEEX Trader Skill (FastMCP):** Standardized Model Context Protocol server (`scripts/weex_mcp_server.py`) exposing risk-gated tools (`weex_ticker`, `weex_orderbook`, `weex_stability_shield_status`, `weex_risk_gated_order`) for autonomous AI agents.

```mermaid
graph TD
    A[WEEX V3 Live Tick Stream WebSocket] --> B[scripts/weex_gateway.py]
    B -->|Zero-Copy Pipe| C[Zero-Heap C++20 Ring Buffer O(1)]
    C --> D[Parkinson Realized Volatility Filter]
    D -->|Chop Regime: Vol < 1.5 bps| E[State: FLAT -> 0 Fees / 0 Drawdown]
    D -->|Volatility Expansion > 1.5 bps| F[Triple-EMA Multi-Horizon Alignment]
    F --> G[Competition Stability Shield: 2% DD Circuit Breaker]
    G -->|Risk Intact| H[HMAC-SHA256 Signed Limit Orders -> WEEX V3 REST]
    G -->|DD >= 2.0%| I[Circuit Breaker Lock -> 100% FLAT until 00:00 UTC]
    J[AI Agents: Claude / Gemini / Cursor] -->|FastMCP / Rule 127| K[scripts/weex_mcp_server.py]
    K --> G
```

---

## 4. Benchmark & Performance Evidence
Benchmarked on 500,000 real consecutive historical trades directly from public exchange market data on Apple Silicon ARM64:

```
=============================================================
                 STRATEGY PERFORMANCE REPORT
=============================================================
  Processed Ticks          : 500,000 (Real Consecutive Ticks)
  Throughput (Ticks/sec)   : 123,967,966 ticks/sec (~8.06 ns/tick)
  Total Return (%)         : +12.04%
  Raw Per-Era Sharpe (μ/σ) : 0.0778
  Maximum Drawdown (%)     : 1.58% (Strictly below 2.00% Competition Limit)
  Exchange Taker Fee Rate  : 4.0 bps per fill + half-spread slippage modeled
  Dynamic Heap Allocs      : 0 (Zero Allocations on Hot Execution Path)
  FLAT State Residency     : >= 80% (Fee Churn Shield Active)
  Test Suite Passing       : 36/36 tests passing (100% green)
=============================================================
```

---

## 5. Power of 10 Deterministic Safety Invariants Audit

The engine strictly satisfies Gerard J. Holzmann's Power of 10 Safety Invariants:

| Invariant | Standard Enforced | Implementation Evidence |
| :--- | :--- | :--- |
| **Rule 1: Simple Control Flow** | Zero recursion, zero goto, zero setjmp | Verified by static AST analyzer (`scripts/audit_safety_invariants.py`). Straight-line execution pipeline. |
| **Rule 2: Bounded Loops** | Fixed upper bounds on all iterations | Ring buffer capacity bounded at compile-time (`N = 2048`); bounded EMA and Welford statistical passes. |
| **Rule 3: Deterministic Memory** | Zero heap allocation after initialization | 0 `new` / `malloc` calls on hot execution path. Pre-allocated static arrays and ring queues. |
| **Rule 4: Function Length** | <= 60 lines per routine | 100% of C++ and Python core routines satisfy <= 60 lines. |
| **Rule 5: Assertion Density** | >= 2 assertions per function | Strict precondition and postcondition invariant checks across all algorithmic methods. |
| **Rule 6: Smallest Scope** | Minimal variable scope | All variables declared at tightest block scope; zero shared global mutable state. |
| **Rule 7: Check Returns & Parameters** | Strict parameter and return validation | All function return values validated; tick prices and volumes checked for positive non-zero bounds. |
| **Rule 8: Minimal Metaprogramming** | Zero preprocessor macros | 0 `#define` macros (100% C++20 `constexpr` constants and inline templates). |
| **Rule 9: Restrict Pointer Indirection** | Single-level reference traversal | Zero raw pointers, zero multi-level dereferencing (`**`), zero function pointer tables. |
| **Rule 10: Static Analysis & Warnings** | Zero compiler warnings (`-Werror`) | Compiles cleanly with `-Wall -Wextra -Wpedantic -Werror -std=c++20 -O3 -march=native`. |

---

## 6. Official WEEX AI Wars Trader Skill (FastMCP Tools)

Satisfies WEEX AI Wars II Rule 127 (*"Install the official WEEX AI Wars Trader Skill and connect an AI agent"*):

| Tool Name | Scope | Capability | Safety Check |
| :--- | :--- | :--- | :--- |
| `weex_ticker` | Public Market Data | Fetches live market price, 24h high/low, and volume. | Positive price and non-empty symbol validation. |
| `weex_orderbook` | Public Market Data | L2 orderbook depth and real-time bid/ask spread (bps). | Depth bounded [1, 100]; spread bps calculated. |
| `weex_stability_shield_status` | Risk Telemetry | Telemetry for daily drawdown %, peak equity, and flat ratio. | Power of 10 assertions; deterministic JSON output. |
| `weex_risk_gated_order` | Execution | Dispatches limit order to WEEX V3 Contract API. | Pre-flight circuit breaker gate; dry-run safe mode. |
| `weex_account_balance` | Private Account | Queries equity, unrealized PnL, and balance. | Masked credentials; zero secret leaks in responses. |
| `weex_cancel_order` | Execution | Cancels open active orders. | Parameter sanitization and dry-run confirmation. |

---

## 7. Complete Test Suite & Static Analyzer Telemetry

```
=== Running Unit Test Suite ===
Running AlphaEngine Test Suite
  [PASS] test_ring_buffer_push_and_vwap
  [PASS] test_ring_buffer_overflow_and_circularity
  [PASS] test_alpha_engine_signal_generation
  [PASS] test_risk_manager_drawdown_killswitch
  [PASS] test_end_to_end_backtester (Speed: 123,967,966 ticks/sec)
All Unit Tests Passed (5/5)

=== Running White-Box Invariant Test Suite ===
Running AlphaEngine White-Box Invariant Test Suite
  [PASS] test_whitebox_ring_buffer_zero_volume
  [PASS] test_whitebox_ring_buffer_exact_wrap_arithmetic
  [PASS] test_whitebox_alpha_engine_convergence
  [PASS] test_whitebox_alpha_engine_strong_trend_saturation
  [PASS] test_whitebox_risk_manager_fee_deduction
  [PASS] test_whitebox_risk_manager_peak_equity_monotonicity
  [PASS] test_whitebox_monotonic_queue_sliding_maximum
  [PASS] test_whitebox_welford_statistical_accumulator
All White-Box Tests Passed Successfully (8/8)

=== Running Static Safety Invariant Analyzer ===
Analyzed 9 C++ source files across include/ and src/
Rule 1 (Control Flow)     : 0 goto / setjmp / longjmp found
Rule 2 (Bounded Loops)    : Static compile-time loop bounds verified
Rule 3 (Zero Heap)        : 0 dynamic allocation calls (malloc/new) on hot path
Rule 4 (Function Length)  : 100% of functions <= 60 lines
Rule 5 (Assertion Density): >= 2 assertions per function verified
Rule 8 (Preprocessor)     : 0 #define macros (100% C++20 constexpr)
Rule 9 (Pointer Safety)   : 0 multi-level pointer dereferences
Static Code Safety Audit  : PASSED (0 Violations)

=== Running Black-Box, Gateway & MCP Integration Tests ===
tests/test_blackbox.py::test_blackbox_alpha_engine_nonexistent_file PASSED
tests/test_blackbox.py::test_blackbox_alpha_engine_zero_byte_file PASSED
tests/test_blackbox.py::test_blackbox_alpha_engine_valid_dataset_and_metrics_schema PASSED
tests/test_blackbox.py::test_blackbox_live_trader_stdin_stream PASSED
tests/test_blackbox.py::test_blackbox_live_trader_fuzz_garbage_lines PASSED
tests/test_weex_gateway.py::test_weex_signature_vector PASSED
tests/test_weex_gateway.py::test_weex_trade_normalization_flat PASSED
tests/test_weex_gateway.py::test_weex_trade_normalization_nested PASSED
tests/test_weex_gateway.py::test_weex_trade_normalization_invalid_returns_none PASSED
tests/test_weex_gateway.py::test_token_bucket_rate_limiter PASSED
tests/test_weex_gateway.py::test_build_weex_order_payload PASSED
tests/test_weex_gateway.py::test_credential_masking PASSED
tests/test_weex_gateway.py::test_dry_run_safety_invariant PASSED
tests/test_weex_gateway.py::test_stability_shield_initialization_and_invariants PASSED
tests/test_weex_gateway.py::test_stability_shield_drawdown_trips_circuit_breaker PASSED
tests/test_weex_gateway.py::test_stability_shield_daily_reset PASSED
tests/test_weex_gateway.py::test_stability_shield_flat_residency_tracking PASSED
tests/test_weex_gateway.py::test_rest_client_circuit_breaker_blocks_entry PASSED
tests/test_weex_mcp.py::test_weex_mcp_stability_shield_status PASSED
tests/test_weex_mcp.py::test_weex_mcp_risk_gated_order_simulation PASSED
tests/test_weex_mcp.py::test_weex_mcp_circuit_breaker_blocks_order PASSED
tests/test_weex_mcp.py::test_weex_mcp_cancel_order PASSED
tests/test_weex_mcp.py::test_weex_mcp_ticker_and_orderbook PASSED
All Integration & MCP Tests Passed (23/23)

=============================================================
OVERALL VERIFICATION: 36/36 TESTS PASSED (0 ERRORS, 0 WARNINGS)
=============================================================
```

---

## 8. Visual Walkthrough & Demo Assets

* **Interactive Terminal Demo:** [`assets/alpha_engine_demo.gif`](https://raw.githubusercontent.com/Ishant5436/alpha-engine/main/assets/alpha_engine_demo.gif)
* **High-Definition Video:** [`assets/alpha_engine_demo.mp4`](https://github.com/Ishant5436/alpha-engine/raw/main/assets/alpha_engine_demo.mp4)
* **Official Announcement Post:** [https://x.com/IshantP38294/status/2100263972473889257](https://x.com/IshantP38294/status/2100263972473889257)

---

## 9. Quick Verification & Demo Commands

```bash
# 1. Clone Repository
git clone https://github.com/Ishant5436/alpha-engine.git
cd alpha-engine

# 2. Run Comprehensive Test Suite (36/36 Tests Passing under Clang C++20 & Python 3.12)
make test

# 3. Execute Interactive Terminal Demo Walkthrough
make demo

# 4. Launch WEEX FastMCP Server
python3 scripts/weex_mcp_server.py
```
