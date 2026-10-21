# Software Quality Risk Register (FMEA Matrix): AlphaEngine
## Conforming to ISO/DIS 9001:2026 Clause 6 (Risk-Based Thinking)

This document tracks identified operational risks, algorithmic failure modes, and automated mitigations for the `alpha-engine` platform.

---

## 1. Risk Evaluation Scale
- **Severity (S):** 1 (Negligible) to 5 (Catastrophic capital loss / memory corruption)
- **Likelihood (L):** 1 (Extremely Rare) to 5 (Frequent without controls)
- **Risk Priority Number (RPN):** $S \times L$ (Scale 1 to 25). RPN $\ge 12$ mandates automated gating.

---

## 2. Failure Modes and Effects Analysis (FMEA)

| Risk ID | Potential Failure Mode | Impact / Effect | Severity (S) | Likelihood (L) | Initial RPN | Automated Mitigation & Quality Control | Residual RPN |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RSK-AE-01** | Peak drawdown limit breach under high market volatility | Uncontrolled portfolio capital depletion | 5 | 4 | **20** | Atomic `RiskManager::update_equity` drawdown check tripping `KillSwitch` liquidation state; verified in `test_risk_manager_drawdown_killswitch` | **3** (S=3, L=1) |
| **RSK-AE-02** | Heap fragmentation or latency spike on hot trading path | Missed fill opportunities, packet drops, GC-like stalls | 5 | 3 | **15** | Rule 3 Invariant: Zero `malloc`/`new` on hot path; pre-allocated stack ring buffers; verified via `scripts/audit_safety_invariants.py` | **2** (S=2, L=1) |
| **RSK-AE-03** | Ring buffer circular index calculation overflow | Memory access out of bounds, segmentation fault | 5 | 3 | **15** | Power-of-two bitwise modulo wrapping (`head & (CAPACITY - 1)`); verified in `test_whitebox_ring_buffer_exact_wrap_arithmetic` | **2** (S=2, L=1) |
| **RSK-AE-04** | Division by zero during zero-volume periods | Floating point NaN propagation corrupting VWAP | 4 | 3 | **12** | Explicit zero volume guard returning previous VWAP; verified in `test_whitebox_ring_buffer_zero_volume` | **2** (S=2, L=1) |
| **RSK-AE-05** | Exchange API rate limit breach on WEEX | IP bans and trading engine lockouts | 4 | 3 | **12** | In-memory token bucket rate limiter with leaky replenishment; verified in `test_token_bucket_rate_limiter` | **2** (S=2, L=1) |
| **RSK-AE-06** | Accidental live order execution during testing/demos | Unauthorized capital deployment on real exchange | 5 | 3 | **15** | Mandatory `dry_run=True` default and credential masking; verified in `test_dry_run_safety_invariant` | **2** (S=2, L=1) |
| **RSK-AE-07** | Micro-timeframe churn and exchange taker fee drag | Capital bleed ("death by a thousand cuts") | 4 | 4 | **16** | Mandatory signal deadband ($\pm 0.15$) and discrete position states; verified in `test_alpha_engine_signal_generation` | **3** (S=3, L=1) |

---

## 3. Review & Verification Frequency
Audited continuously by the automated quality pipeline (`make audit-iso9001`) and static AST analyzers.
