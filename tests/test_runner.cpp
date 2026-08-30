#include <iostream>
#include <vector>
#include <cassert>
#include "types.hpp"
#include "market_data.hpp"
#include "alpha_engine.hpp"
#include "risk_manager.hpp"
#include "backtester.hpp"

#include "test_market_data.cpp"
#include "test_alpha_engine.cpp"
#include "test_risk_manager.cpp"

void test_end_to_end_backtester() {
    std::vector<alpha::Tick> ticks;
    ticks.reserve(5000);

    double price = 100.0;
    for (uint64_t i = 0; i < 5000; ++i) {
        double delta = (i % 200 < 100) ? 0.05 : -0.04;
        price += delta;
        ticks.push_back({
            i * 1000000ULL, // 1ms
            price - 0.01,
            price + 0.01,
            5.0,
            5.0,
            price,
            2.0
        });
    }

    alpha::Backtester bt(10000.0, 0.005, 0.0005, 0.04, 1.0);
    auto metrics = bt.run(ticks.data(), ticks.size());

    assert(metrics.processed_ticks == 5000);
    assert(metrics.ticks_per_second > 1000000.0);
    assert(metrics.max_drawdown_pct <= 6.0);
    assert(std::isfinite(metrics.total_return_pct));

    std::cout << "  [PASS] test_end_to_end_backtester (Speed: " << static_cast<uint64_t>(metrics.ticks_per_second) << " ticks/sec)\n";
}

int main() {
    std::cout << "====================================================\n";
    std::cout << "Running AlphaEngine Test Suite\n";
    std::cout << "====================================================\n";

    test_ring_buffer_push_and_vwap();
    test_ring_buffer_overflow_and_circularity();
    test_alpha_engine_signal_generation();
    test_risk_manager_drawdown_killswitch();
    test_end_to_end_backtester();

    std::cout << "====================================================\n";
    std::cout << "All Unit Tests Passed\n";
    std::cout << "====================================================\n";
    return 0;
}
