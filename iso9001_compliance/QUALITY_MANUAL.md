# Software Quality Management System (QMS) Manual: AlphaEngine
## Conforming to ISO/DIS 9001:2026 (Draft International Standard)

---

### 1. Scope & Application
This Quality Manual establishes the Software Quality Management System (QMS) policies, procedures, and deterministic controls implemented across the `alpha-engine` repository. It formalizes quality engineering practices for low-latency algorithmic trading, high-frequency market data ingestion, and risk containment under **ISO/DIS 9001:2026**.

---

### 2. Clause 4: Context of the Organization & Digital Infrastructure
- **4.1 Organizational Context & Computing Architecture:** The engine operates on Apple Silicon ARM64 architecture (Darwin macOS), utilizing optimized C++20 instructions (`-O3 -march=native`) and zero-heap hot paths for predictable deterministic execution.
- **4.2 Stakeholder Expectations:** Exchange venues (WEEX, Binance), liquidity providers, and trading operators require zero unhandled panics, zero memory leaks, sub-microsecond tick processing, and strict risk limits.
- **4.3 Scope of the QMS:** Covers all C++20 core execution components (`RingBuffer`, `AlphaEngine`, `RiskManager`, `WelfordAccumulator`, `MonotonicQueue`), Python gateway services (`WeexGateway`, `StabilityShield`), and FastMCP tool interfaces (`WeexMCPServer`).
- **4.4 QMS and Automated Verification:** Quality gates are encapsulated as executable targets in [`Makefile`](file:///Users/ishantpanchal/alpha-engine/Makefile), integrating unit tests, whitebox invariant checks, ASan/UBSan sanitizers, and static AST analyzers.

---

### 3. Clause 5: Leadership & Quality Culture
- **5.1 Leadership & Commitment:** Engineering leadership enforces a strict **Zero Completion Claims Without Verification** policy. No commit or release is recognized without 100% green test and sanitizer logs.
- **5.2 Quality Policy:** The project commits to zero dynamic memory allocation on hot paths, deterministic bounded loops, mathematical invariant preservation, and institutional drawdown defense.
- **5.3 Organizational Roles & Responsibilities:** Autonomous static AST analyzers, compiler sanitizers, and unit test runners act as automated quality gatekeepers.

---

### 4. Clause 6: Planning & Risk-Based Thinking
- **6.1 Actions to Address Risks & Opportunities:** The QMS maintains an active [`RISK_REGISTER.md`](file:///Users/ishantpanchal/alpha-engine/iso9001_compliance/RISK_REGISTER.md) analyzing failure modes such as drawdown limit breaches, ring buffer overflows, floating-point drift, and exchange API rate limit penalties.
- **6.2 Quality Objectives:**
  - *Throughput SLA:* Tick ingestion and VWAP processing speed exceeding 10,000,000 ticks/sec (benchmark achieved: >120,000,000 ticks/sec).
  - *Memory Safety:* Zero memory leaks, zero buffer overruns, zero undefined behavior verified under AddressSanitizer and UndefinedBehaviorSanitizer.
  - *Risk Containment:* Hard-coded global killswitch triggering immediate position liquidation upon hitting the maximum drawdown ceiling (default 5.0%).
  - *Power of 10 Invariants:* 100% compliance with Gerard J. Holzmann's safety-critical coding standards (functions $\le 60$ lines, assertion density $\ge 2$).

---

### 5. Clause 7: Support & Tool Qualification
- **7.1 Resources & Qualified Compilers:**
  - C++ Compiler: Clang++ supporting C++20 (`-std=c++20`).
  - Runtime Sanitizers: LLVM AddressSanitizer (`-fsanitize=address`) & UBSan (`-fsanitize=undefined`).
  - Python Environment: Python 3.14 with pytest, anyio, and fastmcp.
  - Static AST Analyzers: Python AST validator for Power of 10 safety invariants.
- **7.2 Competence & Training:** Full technical architecture and execution guides documented in [`README.md`](file:///Users/ishantpanchal/alpha-engine/README.md) and [`ARCHITECTURE.md`](file:///Users/ishantpanchal/alpha-engine/ARCHITECTURE.md).
- **7.5 Documented Information:** Test reports, sanitizer output, and AST analyzer matrices are retained as verification artifacts.

---

### 6. Clause 8: Operational Planning and Control (Software V&V)
- **8.1 Verification and Validation Protocol:**
  - *Verification (Unit & Whitebox):* Mathematical verification of circular wrap-around indexing, VWAP calculation, Welford variance accumulation, and signal saturation.
  - *Validation (Sanitizers):* ASan/UBSan memory execution (`make asan`) confirming zero heap corruptions.
  - *Integration (Gateway & MCP):* Blackbox validation of exchange order formatting, token-bucket rate limiting, and circuit-breaker tripping.
- **8.7 Control of Non-conforming Outputs:** Any assertion failure, sanitizer panic, or invariant violation halts compilation and deployment immediately.

---

### 7. Clause 9: Performance Evaluation
- **9.1 Monitoring & Measurement:** Continuous performance profiling of execution speed (`ticks/sec`) and memory residency.
- **9.2 Internal Audit:** Automated static & runtime audit executed via [`scripts/audit_iso9001_compliance.py`](file:///Users/ishantpanchal/alpha-engine/scripts/audit_iso9001_compliance.py).
- **9.3 Safety Auditing:** AST invariant checker [`scripts/audit_safety_invariants.py`](file:///Users/ishantpanchal/alpha-engine/scripts/audit_safety_invariants.py) analyzing all C++ source files.

---

### 8. Clause 10: Continual Improvement
- **10.1 Non-conformity and Corrective Action:** Latency degradation or failed assertions trigger root cause investigation, followed by regression test inclusion in `test_whitebox.cpp`.
- **10.2 Continual Improvement Cycle:** Regular profiling, deadband calibration against friction drag, and elimination of trade churning on micro-intervals.
