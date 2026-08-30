#include <cassert>
#include <iostream>
#include "risk_manager.hpp"

void test_risk_manager_drawdown_killswitch() {
    alpha::RiskManager rm(10000.0, 0.04, 3.0, 0.0); // 0 fee for isolated circuit-breaker unit test
    assert(rm.position().current_equity == 10000.0);
    assert(!rm.position().is_liquidated);

    // Enter Long 10 units at 100.0 ($1000 notional)
    rm.execute_fill(alpha::OrderSide::BUY, 100.0, 10.0);
    assert(rm.position().size == 10.0);
    assert(rm.position().entry_price == 100.0);

    // Price drops to 95.0 -> loss of $50 (equity 9950 -> 0.5% drawdown)
    bool ok = rm.update_pnl(95.0);
    assert(ok);
    assert(!rm.position().is_liquidated);
    assert(rm.position().current_equity == 9950.0);

    // Price drops to 50.0 -> loss of $500 (equity 9500 -> 5% drawdown > 4% limit)
    ok = rm.update_pnl(50.0);
    assert(!ok); // Kill-switch triggered
    assert(rm.position().is_liquidated);
    assert(rm.position().size == 0.0); // Flattened

    // Subsequent orders must be rejected
    alpha::AlphaSignal buy_sig{0.8, 0.5, 0.001, 0.8, true};
    double target = rm.calculate_target_position(buy_sig, 50.0);
    assert(target == 0.0);

    std::cout << "  [PASS] test_risk_manager_drawdown_killswitch\n";
}
