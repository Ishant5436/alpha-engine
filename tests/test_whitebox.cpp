#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "types.hpp"
#include "market_data.hpp"
#include "alpha_engine.hpp"
#include "risk_manager.hpp"
#include "monotonic_queue.hpp"
#include "welford_accumulator.hpp"

using namespace alpha;

// ==============================================================================
// 1. Ring Buffer Mathematical & Boundary Invariants (White-Box)
// ==============================================================================

void test_whitebox_ring_buffer_zero_volume() {
    MarketDataRingBuffer<8> rb;
    assert(rb.vwap() == 0.0);

    Tick zero_vol_tick{1000, 100.0, 100.2, 0.0, 0.0, 100.1, 0.0};
    assert(rb.push(zero_vol_tick));
    assert(rb.vwap() == 0.0); // Must not divide by zero
}

void test_whitebox_ring_buffer_exact_wrap_arithmetic() {
    MarketDataRingBuffer<4> rb;
    // Push 4 ticks with known prices and volumes
    rb.push({1000, 10.0, 10.2, 1.0, 1.0, 10.0, 2.0}); // PV = 20, Vol = 2
    rb.push({2000, 20.0, 20.2, 1.0, 1.0, 20.0, 3.0}); // PV = 60, Vol = 3
    rb.push({3000, 30.0, 30.2, 1.0, 1.0, 30.0, 5.0}); // PV = 150, Vol = 5
    rb.push({4000, 40.0, 40.2, 1.0, 1.0, 40.0, 10.0}); // PV = 400, Vol = 10
    // Total PV = 630, Vol = 20 -> VWAP = 31.5
    assert(std::abs(rb.vwap() - 31.5) < 1e-9);

    // Push 5th tick -> evicts 1st tick (PV 20, Vol 2)
    rb.push({5000, 50.0, 50.2, 1.0, 1.0, 50.0, 2.0}); // PV = 100, Vol = 2
    // New total PV = 630 - 20 + 100 = 710, Vol = 20 - 2 + 2 = 20 -> VWAP = 35.5
    assert(std::abs(rb.vwap() - 35.5) < 1e-9);
    assert(rb.size() == 4);
    assert(rb.latest().last_price == 50.0);
    assert(rb[3].last_price == 20.0); // Oldest remaining
}

// ==============================================================================
// 2. Alpha Engine Signal Bounds & Exponential Moving Average (White-Box)
// ==============================================================================

void test_whitebox_alpha_engine_convergence() {
    MultiFactorAlphaEngine engine(0.1, 0.05, 0.01);
    MarketDataRingBuffer<MAX_RING_CAPACITY> rb;

    // Feed identical ticks -> EMA must converge to the constant price
    for (uint64_t i = 0; i < 500; ++i) {
        rb.push({i * 1000, 100.0, 100.0, 1.0, 1.0, 100.0, 1.0});
        AlphaSignal sig = engine.update(rb);
        if (i > 300) {
            assert(sig.is_valid);
            assert(std::abs(sig.fast_ema - 100.0) < 0.01);
            assert(std::abs(sig.med_ema - 100.0) < 0.01);
            assert(std::abs(sig.slow_ema - 100.0) < 0.05);
            assert(std::abs(sig.composite_signal) < 0.1); // No trend
        }
    }
}

void test_whitebox_alpha_engine_strong_trend_saturation() {
    MultiFactorAlphaEngine engine(0.1, 0.05, 0.01);
    MarketDataRingBuffer<MAX_RING_CAPACITY> rb;

    // Strong upward trend
    for (uint64_t i = 0; i < 200; ++i) {
        double p = 100.0 + i * 2.0;
        rb.push({i * 1000, p - 0.01, p + 0.01, 1.0, 1.0, p, 1.0});
        AlphaSignal sig = engine.update(rb);
        if (i >= ALPHA_WARMUP_TICKS) {
            assert(sig.is_valid);
            assert(sig.composite_signal > 0.0); // Bullish
            assert(sig.fast_ema > sig.med_ema);
            assert(sig.med_ema > sig.slow_ema);
        }
    }
}

// ==============================================================================
// 3. Risk Manager Fee Math & Atomic Position Invariants (White-Box)
// ==============================================================================

void test_whitebox_risk_manager_fee_deduction() {
    // 4 bps fee = 0.0004
    RiskManager rm(10000.0, 0.10, 2.0, 0.0004);
    assert(rm.cumulative_fees() == 0.0);

    // Buy 10 BTC at $50,000 notional = $500,000 -> fee = 500000 * 0.0004 = $200.00
    // Realized PnL starts at -$200 (fees deducted immediately)
    rm.execute_fill(OrderSide::BUY, 50000.0, 10.0);
    assert(std::abs(rm.cumulative_fees() - 200.0) < 1e-6);
    assert(std::abs(rm.position().realized_pnl - (-200.0)) < 1e-6);

    // Sell 10 BTC at $55,000 notional = $550,000 -> fee = 550000 * 0.0004 = $220.00
    // Gross PnL = (55000 - 50000) * 10 = +$50,000
    // Net Realized PnL = 50000 - 200 - 220 = +$49,580
    rm.execute_fill(OrderSide::SELL, 55000.0, 10.0);
    assert(std::abs(rm.cumulative_fees() - 420.0) < 1e-6);
    assert(std::abs(rm.position().realized_pnl - 49580.0) < 1e-6);
    assert(rm.position().size == 0.0); // Flat
}

void test_whitebox_risk_manager_peak_equity_monotonicity() {
    RiskManager rm(10000.0, 0.10, 1.0, 0.0);
    assert(rm.position().peak_equity == 10000.0);

    rm.execute_fill(OrderSide::BUY, 100.0, 10.0);
    rm.update_pnl(120.0); // Equity = 10000 + 200 = 10200
    assert(rm.position().peak_equity == 10200.0);

    rm.update_pnl(110.0); // Equity drops to 10100 -> peak MUST NOT decrease
    assert(rm.position().peak_equity == 10200.0);

    rm.update_pnl(150.0); // Equity rises to 10500 -> new peak
    assert(rm.position().peak_equity == 10500.0);
}

void test_whitebox_monotonic_queue_sliding_maximum() {
    MonotonicQueue<double, 8, std::greater<double>> mq;
    assert(mq.empty());

    // Window size = 3 ticks. Feed values: [10, 5, 12, 8, 15]
    mq.push(10.0, 0); // Window: [10] -> max = 10
    assert(mq.top() == 10.0);

    mq.push(5.0, 1);  // Window: [10, 5] -> max = 10
    assert(mq.top() == 10.0);

    mq.push(12.0, 2); // Window: [10, 5, 12] -> max = 12 (10 and 5 evicted from queue)
    assert(mq.top() == 12.0);

    // Slide window: min_valid_index = 1 (removes index 0)
    mq.pop_expired(1);
    assert(mq.top() == 12.0);

    mq.push(8.0, 3);  // Window: [5, 12, 8] -> max = 12
    mq.pop_expired(2); // Slide window: min_valid_index = 2 -> window: [12, 8]
    assert(mq.top() == 12.0);

    mq.pop_expired(3); // Slide window: min_valid_index = 3 -> removes index 2 (12)
    assert(mq.top() == 8.0); // Now 8.0 is max

    mq.push(15.0, 4); // Window: [8, 15] -> max = 15
    assert(mq.top() == 15.0);
}

void test_whitebox_welford_statistical_accumulator() {
    WelfordAccumulator acc;
    assert(acc.count() == 0);
    assert(acc.variance() == 0.0);

    // Sample: [2, 4, 4, 4, 5, 5, 7, 9]
    // Mean = 5.0, Sample Variance = 4.571428... (32 / 7)
    const std::vector<double> samples = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    for (double x : samples) {
        acc.update(x);
    }

    assert(acc.count() == 8);
    assert(std::abs(acc.mean() - 5.0) < 1e-9);
    assert(std::abs(acc.variance() - (32.0 / 7.0)) < 1e-9);
    assert(std::abs(acc.stdev() - std::sqrt(32.0 / 7.0)) < 1e-9);
}

int main() {
    std::cout << "====================================================\n";
    std::cout << "Running AlphaEngine White-Box Invariant Test Suite\n";
    std::cout << "====================================================\n";

    test_whitebox_ring_buffer_zero_volume();
    std::cout << "  [PASS] test_whitebox_ring_buffer_zero_volume\n";

    test_whitebox_ring_buffer_exact_wrap_arithmetic();
    std::cout << "  [PASS] test_whitebox_ring_buffer_exact_wrap_arithmetic\n";

    test_whitebox_alpha_engine_convergence();
    std::cout << "  [PASS] test_whitebox_alpha_engine_convergence\n";

    test_whitebox_alpha_engine_strong_trend_saturation();
    std::cout << "  [PASS] test_whitebox_alpha_engine_strong_trend_saturation\n";

    test_whitebox_risk_manager_fee_deduction();
    std::cout << "  [PASS] test_whitebox_risk_manager_fee_deduction\n";

    test_whitebox_risk_manager_peak_equity_monotonicity();
    std::cout << "  [PASS] test_whitebox_risk_manager_peak_equity_monotonicity\n";

    test_whitebox_monotonic_queue_sliding_maximum();
    std::cout << "  [PASS] test_whitebox_monotonic_queue_sliding_maximum\n";

    test_whitebox_welford_statistical_accumulator();
    std::cout << "  [PASS] test_whitebox_welford_statistical_accumulator\n";

    std::cout << "====================================================\n";
    std::cout << "All White-Box Tests Passed Successfully!\n";
    std::cout << "====================================================\n";
    return 0;
}
