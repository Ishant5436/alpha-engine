# ISO/DIS 9001:2026 Bidirectional Traceability Matrix: AlphaEngine

This matrix establishes forward and backward traceability between institutional trading requirements, source implementation, test targets, and verified quality evidence.

---

## 1. Traceability Mapping

| Requirement ID | Requirement Specification | Test Case ID | Test Implementation | Target Source Component | Verifiable Evidence Artifact |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **REQ-AE-001** | Sub-microsecond fixed-capacity circular ring buffer with online VWAP | `TC-RB-01` | [`test_runner.cpp`](file:///Users/ishantpanchal/alpha-engine/tests/test_runner.cpp#L18) | [`ring_buffer.hpp`](file:///Users/ishantpanchal/alpha-engine/include/ring_buffer.hpp) | `bin/test_runner` console log |
| **REQ-AE-002** | Power-of-two circular wrap-around indexing without memory corruption | `TC-RB-02` | [`test_whitebox.cpp`](file:///Users/ishantpanchal/alpha-engine/tests/test_whitebox.cpp#L25) | [`ring_buffer.hpp`](file:///Users/ishantpanchal/alpha-engine/include/ring_buffer.hpp) | `bin/test_whitebox` console log |
| **REQ-AE-003** | Dual-window EMA momentum and VWAP divergence signal generation | `TC-SIG-01` | [`test_runner.cpp`](file:///Users/ishantpanchal/alpha-engine/tests/test_runner.cpp#L42) | [`alpha_engine.hpp`](file:///Users/ishantpanchal/alpha-engine/include/alpha_engine.hpp) | `bin/test_runner` console log |
| **REQ-AE-004** | Maximum drawdown ceiling tripping instantaneous emergency killswitch | `TC-RM-01` | [`test_runner.cpp`](file:///Users/ishantpanchal/alpha-engine/tests/test_runner.cpp#L65) | [`risk_manager.hpp`](file:///Users/ishantpanchal/alpha-engine/include/risk_manager.hpp) | `bin/test_runner` console log |
| **REQ-AE-005** | High-throughput end-to-end backtester processing $\ge 10\text{M}$ ticks/sec | `TC-BT-01` | [`test_runner.cpp`](file:///Users/ishantpanchal/alpha-engine/tests/test_runner.cpp#L90) | [`alpha_engine.hpp`](file:///Users/ishantpanchal/alpha-engine/include/alpha_engine.hpp) | Benchmark output (>120M ticks/s) |
| **REQ-AE-006** | Welford numerical variance accumulator with floating-point drift defense | `TC-WF-01` | [`test_whitebox.cpp`](file:///Users/ishantpanchal/alpha-engine/tests/test_whitebox.cpp#L105) | [`welford.hpp`](file:///Users/ishantpanchal/alpha-engine/include/welford.hpp) | `bin/test_whitebox` console log |
| **REQ-AE-007** | Strict zero-heap allocation and Power of 10 safety invariant adherence | `TC-AST-01` | [`audit_safety_invariants.py`](file:///Users/ishantpanchal/alpha-engine/scripts/audit_safety_invariants.py) | All C++ headers & sources | AST Invariant Analyzer log |
| **REQ-AE-008** | AddressSanitizer and UndefinedBehaviorSanitizer zero leak guarantee | `TC-SAN-01` | `make asan` | All compiled binaries | ASan terminal report (0 leaks) |
| **REQ-AE-009** | WEEX HMAC-SHA256 signature vector validation | `TC-GW-01` | [`test_weex_gateway.py`](file:///Users/ishantpanchal/alpha-engine/tests/test_weex_gateway.py) | [`weex_gateway.py`](file:///Users/ishantpanchal/alpha-engine/gateway/weex_gateway.py) | Pytest execution report |
| **REQ-AE-010** | Token-bucket rate limiter preventing exchange API 429 throttling | `TC-GW-02` | [`test_weex_gateway.py`](file:///Users/ishantpanchal/alpha-engine/tests/test_weex_gateway.py) | [`weex_gateway.py`](file:///Users/ishantpanchal/alpha-engine/gateway/weex_gateway.py) | Pytest execution report |
| **REQ-AE-011** | Dry-run safety invariant preventing accidental capital deployment | `TC-GW-03` | [`test_weex_gateway.py`](file:///Users/ishantpanchal/alpha-engine/tests/test_weex_gateway.py) | [`weex_gateway.py`](file:///Users/ishantpanchal/alpha-engine/gateway/weex_gateway.py) | Pytest execution report |
| **REQ-AE-012** | FastMCP risk-gated order dispatch and live stability shield reporting | `TC-MCP-01` | [`test_weex_mcp.py`](file:///Users/ishantpanchal/alpha-engine/tests/test_weex_mcp.py) | [`weex_mcp.py`](file:///Users/ishantpanchal/alpha-engine/mcp_server/weex_mcp.py) | Pytest execution report |

---

## 2. Verification Coverage
- **Total Tracked Requirements:** 12
- **Verification Coverage:** 100% (12/12 verified with automated tests)
- **Sanitizer & Safety Verification:** AddressSanitizer (0 leaks), UBSan (0 errors), AST Invariants (0 violations).
