#include <cassert>
#include <iostream>
#include "alpha_engine.hpp"

void test_alpha_engine_signal_generation() {
    alpha::MarketDataRingBuffer<alpha::MAX_RING_CAPACITY> ring;
    alpha::MultiFactorAlphaEngine engine(0.20, 0.05, 0.001);

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

    // Ticks 30..70: Strong upward trend
    for (int i = 30; i < 70; ++i) {
        double price = 100.0 + (i - 30) * 0.5;
        alpha::Tick t{static_cast<uint64_t>(i * 1000), price, price + 0.2, 10.0, 10.0, price + 0.1, 10.0};
        ring.push(t);
        auto sig = engine.update(ring);
        assert(sig.is_valid);
        assert(sig.composite_signal >= -1.0 && sig.composite_signal <= 1.0);
    }

    auto final_sig = engine.update(ring);
    assert(final_sig.is_valid);
    std::cout << "    Debug signal: mom=" << final_sig.momentum_score 
              << ", mr=" << final_sig.mean_reversion_score 
              << ", comp=" << final_sig.composite_signal 
              << ", vol=" << final_sig.volatility_regime << "\n";
    assert(final_sig.momentum_score > 0.0); // Trend is positive
    assert(final_sig.composite_signal > 0.0);

    std::cout << "  [PASS] test_alpha_engine_signal_generation\n";
}
