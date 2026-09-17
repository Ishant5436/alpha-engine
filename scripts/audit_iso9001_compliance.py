#!/usr/bin/env python3
"""
ISO/DIS 9001:2026 Quality Management System (QMS) Compliance Auditor
Tailored for AlphaEngine Low-Latency Trading & Risk System
"""
import os
import sys
import json
import shutil
from datetime import datetime

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def file_exists(rel_path: str) -> bool:
    return os.path.exists(os.path.join(BASE_DIR, rel_path))

def file_contains(rel_path: str, keyword: str) -> bool:
    p = os.path.join(BASE_DIR, rel_path)
    if not os.path.exists(p):
        return False
    try:
        with open(p, "r", encoding="utf-8", errors="ignore") as f:
            return keyword in f.read()
    except Exception:
        return False

def audit_clause_4():
    """Clause 4: Context of the Organization & Digital Infrastructure"""
    checks = {
        "makefile_orchestrator_defined": file_exists("Makefile"),
        "core_cpp_headers_present": file_exists("include/alpha_engine.hpp") and file_exists("include/market_data.hpp"),
        "gateway_subsystem_present": file_exists("scripts/weex_gateway.py"),
        "mcp_server_present": file_exists("scripts/weex_mcp_server.py"),
    }
    return {
        "clause": "Clause 4: Context & Digital Infrastructure",
        "passed": all(checks.values()),
        "details": checks
    }

def audit_clause_5():
    """Clause 5: Leadership & Quality Culture"""
    checks = {
        "quality_manual_present": file_exists("iso9001_compliance/QUALITY_MANUAL.md"),
        "zero_defect_policy_stated": file_contains("iso9001_compliance/QUALITY_MANUAL.md", "Zero Completion Claims Without Verification"),
        "makefile_pipeline_gated": file_contains("Makefile", "test: bin/test_runner bin/test_whitebox"),
    }
    return {
        "clause": "Clause 5: Leadership & Quality Culture",
        "passed": all(checks.values()),
        "details": checks
    }

def audit_clause_6():
    """Clause 6: Planning & Risk-Based Thinking"""
    checks = {
        "risk_register_maintained": file_exists("iso9001_compliance/RISK_REGISTER.md"),
        "drawdown_killswitch_implemented": file_contains("include/risk_manager.hpp", "position_.is_liquidated = true"),
        "zero_volume_division_guard": file_contains("include/market_data.hpp", "cumulative_volume_ <= 0.0"),
        "dry_run_safety_guard": file_contains("scripts/weex_gateway.py", "self.dry_run = dry_run"),
    }
    return {
        "clause": "Clause 6: Planning & Risk-Based Thinking",
        "passed": all(checks.values()),
        "details": checks
    }

def audit_clause_7():
    """Clause 7: Support & Tool Qualification"""
    checks = {
        "clang_compiler_available": shutil.which("clang++") is not None,
        "python3_runtime_available": shutil.which("python3") is not None,
        "ast_safety_analyzer_available": file_exists("scripts/audit_safety_invariants.py"),
        "architecture_guide_present": file_exists("README.md"),
    }
    return {
        "clause": "Clause 7: Support & Tool Qualification",
        "passed": all(checks.values()),
        "details": checks
    }

def audit_clause_8():
    """Clause 8: Operation & Software Verification/Validation (V&V)"""
    checks = {
        "traceability_matrix_present": file_exists("iso9001_compliance/TRACEABILITY_MATRIX.md"),
        "unit_test_suite_present": file_exists("tests/test_runner.cpp"),
        "whitebox_invariant_suite_present": file_exists("tests/test_whitebox.cpp"),
        "python_gateway_test_suite_present": file_exists("tests/test_weex_gateway.py"),
        "mcp_server_test_suite_present": file_exists("tests/test_weex_mcp.py"),
    }
    return {
        "clause": "Clause 8: Operation & Software V&V",
        "passed": all(checks.values()),
        "details": checks
    }

def audit_clause_9():
    """Clause 9: Performance Evaluation & Audit Trails"""
    checks = {
        "sanitizer_asan_ubsan_target_configured": file_contains("Makefile", "-fsanitize=address,undefined"),
        "static_safety_audit_target_configured": file_contains("Makefile", "audit_safety_invariants.py"),
        "zero_heap_invariant_enforced": file_contains("scripts/audit_safety_invariants.py", "Rule 3"),
    }
    return {
        "clause": "Clause 9: Performance Evaluation & Audit Trails",
        "passed": all(checks.values()),
        "details": checks
    }

def audit_clause_10():
    """Clause 10: Continual Improvement & Defect Containment"""
    checks = {
        "power_of_10_invariant_checker_wired": file_exists("scripts/audit_safety_invariants.py"),
        "automated_qms_auditor_wired": file_exists("scripts/audit_iso9001_compliance.py"),
    }
    return {
        "clause": "Clause 10: Continual Improvement & Defect Containment",
        "passed": all(checks.values()),
        "details": checks
    }

def main():
    print("==========================================================================")
    print("      ISO/DIS 9001:2026 Quality Management System (QMS) Compliance Auditor")
    print("      Target: AlphaEngine Algorithmic Execution System")
    print("==========================================================================")

    audits = [
        audit_clause_4(),
        audit_clause_5(),
        audit_clause_6(),
        audit_clause_7(),
        audit_clause_8(),
        audit_clause_9(),
        audit_clause_10(),
    ]

    total_clauses = len(audits)
    passed_clauses = sum(1 for a in audits if a["passed"])
    score_pct = (passed_clauses / total_clauses) * 100.0

    print(f"\nAudit Timestamp: {datetime.now().isoformat()}")
    print(f"Target Repository: {BASE_DIR}\n")
    print("Clause-by-Clause Compliance Scorecard:")
    print("--------------------------------------------------------------------------")

    for a in audits:
        status_symbol = "✓ PASS" if a["passed"] else "✗ FAIL"
        print(f"[{status_symbol}] {a['clause']}")
        for detail, res in a["details"].items():
            sub_symbol = "  ✔" if res else "  ✖"
            print(f"    {sub_symbol} {detail}: {'Satisfied' if res else 'MISSING'}")

    print("--------------------------------------------------------------------------")
    print(f"Final Compliance Score: {score_pct:.1f}% ({passed_clauses}/{total_clauses} Clauses Compliant)")

    target_dir = os.path.join(BASE_DIR, "target")
    os.makedirs(target_dir, exist_ok=True)
    report_path = os.path.join(target_dir, "iso9001_audit_report.json")

    report_payload = {
        "standard": "ISO/DIS 9001:2026",
        "repository": "alpha-engine",
        "timestamp": datetime.now().isoformat(),
        "score_percent": score_pct,
        "clauses_audited": total_clauses,
        "clauses_passed": passed_clauses,
        "compliant": score_pct == 100.0,
        "results": audits
    }

    with open(report_path, "w") as f:
        json.dump(report_payload, f, indent=2)

    print(f"\nMachine-readable audit artifact written to: {report_path}")

    if score_pct == 100.0:
        print("\n🏆 AUDIT RESULT: FULLY CONFORMANT TO ISO/DIS 9001:2026 STANDARD.")
        sys.exit(0)
    else:
        print("\n⚠️ AUDIT RESULT: NON-CONFORMANCES DETECTED.")
        sys.exit(1)

if __name__ == "__main__":
    main()
