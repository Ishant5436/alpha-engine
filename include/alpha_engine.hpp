#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>
#include "types.hpp"
#include "market_data.hpp"

namespace alpha {

constexpr std::size_t ALPHA_WARMUP_TICKS = 30;

struct AlphaSignal {
    double momentum_score{0.0};       // Normalized [-1.0, 1.0]
    double mean_reversion_score{0.0}; // Normalized [-1.0, 1.0]
    double raw_trend_spread{0.0};     // (fast_ema - slow_ema) / price
    double composite_signal{0.0};     // Actionable signal [-1.0, 1.0]
    bool is_valid{false};
};

class MultiFactorAlphaEngine {
public:
    constexpr MultiFactorAlphaEngine(double fast_alpha = 0.005, double slow_alpha = 0.0005) noexcept
        : fast_alpha_(fast_alpha), slow_alpha_(slow_alpha),
          fast_ema_(0.0), slow_ema_(0.0), is_initialized_(false) {
        assert(fast_alpha_ > slow_alpha_);
        assert(slow_alpha_ > 0.0);
    }

    AlphaSignal update(const MarketDataRingBuffer<MAX_RING_CAPACITY>& ring) noexcept {
        assert(ring.size() > 0);
        AlphaSignal sig{};
        if (ring.size() < ALPHA_WARMUP_TICKS) {
            return sig;
        }

        const double current_price = ring.latest().last_price;
        assert(current_price > 0.0);

        if (!is_initialized_) {
            fast_ema_ = current_price;
            slow_ema_ = current_price;
            is_initialized_ = true;
        } else {
            fast_ema_ = fast_alpha_ * current_price + (1.0 - fast_alpha_) * fast_ema_;
            slow_ema_ = slow_alpha_ * current_price + (1.0 - slow_alpha_) * slow_ema_;
        }

        // Relative trend spread (fast EMA vs slow EMA)
        sig.raw_trend_spread = (fast_ema_ - slow_ema_) / current_price;
        sig.momentum_score = std::clamp(sig.raw_trend_spread / 0.0010, -1.0, 1.0);

        const double vwap_val = ring.vwap();
        sig.mean_reversion_score = std::clamp((vwap_val - current_price) / (current_price * 0.0010), -1.0, 1.0);

        // Actionable signal with deadband filter to eliminate noise
        if (sig.raw_trend_spread > 0.00020) {
            sig.composite_signal = 1.0; // Strong Bull Trend
        } else if (sig.raw_trend_spread < -0.00020) {
            sig.composite_signal = -1.0; // Strong Bear Trend
        } else {
            sig.composite_signal = 0.0;  // Neutral / Range
        }

        sig.is_valid = true;
        assert(sig.composite_signal >= -1.0 && sig.composite_signal <= 1.0);
        assert(sig.is_valid);
        return sig;
    }

private:
    double fast_alpha_{0.005};
    double slow_alpha_{0.0005};
    double fast_ema_{0.0};
    double slow_ema_{0.0};
    bool is_initialized_{false};
};

} // namespace alpha
