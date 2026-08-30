#include <cassert>
#include <iostream>
#include "alpha_engine.hpp"

void test_alpha_engine_signal_generation() {
    alpha::MarketDataRingBuffer<alpha::MAX_RING_CAPACITY> ring;
    alpha::MultiFactorAlphaEngine engine(0.08, 0.02, 0.005);

    // Initial 100 ticks: warmup period
    for (int i = 0; i < 100; ++i) {
        alpha::Tick t{static_cast<uint64_t>(i * 1000), 100.0, 100.1, 10.0, 10.0, 100.05, 5.0};
        ring.push(t);
        auto sig = engine.update(ring);
        if (i < 99) {
            assert(!sig.is_valid);
        } else {
            assert(sig.is_valid);
        }
    }

    // Ticks 100..200: Strong upward trend with volatility expansion
    for (int i = 100; i < 200; ++i) {
        double price = 100.0 + (i - 100) * 0.8;
        alpha::Tick t{static_cast<uint64_t>(i * 1000), price - 0.2, price + 0.2, 10.0, 10.0, price, 10.0};
        ring.push(t);
        auto sig = engine.update(ring);
        assert(sig.is_valid);
        assert(sig.composite_signal >= -1.0 && sig.composite_signal <= 1.0);
    }

    auto final_sig = engine.update(ring);
    assert(final_sig.is_valid);
    assert(final_sig.trend_spread_bps > 0.0); // Trend is positive
    assert(final_sig.composite_signal == 1.0);

    std::cout << "  [PASS] test_alpha_engine_signal_generation\n";
}
