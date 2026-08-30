#include <cassert>
#include <iostream>
#include "alpha_engine.hpp"

void test_alpha_engine_signal_generation() {
    alpha::MarketDataRingBuffer<alpha::MAX_RING_CAPACITY> ring;
    alpha::MultiFactorAlphaEngine engine(0.05, 0.005);

    // Initial 30 ticks: warmup period
    for (int i = 0; i < 30; ++i) {
        alpha::Tick t{static_cast<uint64_t>(i * 1000), 100.0, 100.1, 10.0, 10.0, 100.05, 5.0};
        ring.push(t);
        auto sig = engine.update(ring);
        if (i < 29) {
            assert(!sig.is_valid);
        } else {
            assert(sig.is_valid);
        }
    }

    // Ticks 30..80: Strong upward trend
    for (int i = 30; i < 80; ++i) {
        double price = 100.0 + (i - 30) * 0.5;
        alpha::Tick t{static_cast<uint64_t>(i * 1000), price, price + 0.2, 10.0, 10.0, price + 0.1, 10.0};
        ring.push(t);
        auto sig = engine.update(ring);
        assert(sig.is_valid);
        assert(sig.composite_signal >= -1.0 && sig.composite_signal <= 1.0);
    }

    auto final_sig = engine.update(ring);
    assert(final_sig.is_valid);
    assert(final_sig.raw_trend_spread > 0.0); // Trend is positive
    assert(final_sig.composite_signal == 1.0);

    std::cout << "  [PASS] test_alpha_engine_signal_generation\n";
}
