#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>
#include "types.hpp"
#include "market_data.hpp"
#include "welford_accumulator.hpp"

namespace alpha {

constexpr std::size_t ALPHA_WARMUP_TICKS = 100;

struct AlphaSignal {
    double fast_ema{0.0};
    double med_ema{0.0};
    double slow_ema{0.0};
    double realized_vol{0.0};
    double raw_trend_spread{0.0};
    double trend_spread_bps{0.0};
    double composite_signal{0.0}; // 1.0 = Bull, -1.0 = Bear, 0.0 = Flat
    bool is_valid{false};
};

class MultiFactorAlphaEngine {
public:
    constexpr MultiFactorAlphaEngine(double fast_alpha = 0.005, double med_alpha = 0.001, double slow_alpha = 0.0005) noexcept
        : fast_alpha_(fast_alpha), med_alpha_(med_alpha), slow_alpha_(slow_alpha),
          fast_ema_(0.0), med_ema_(0.0), slow_ema_(0.0), prev_price_(0.0),
          vol_acc_(), is_initialized_(false) {
        assert(fast_alpha_ > med_alpha_);
        assert(med_alpha_ > slow_alpha_);
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
            med_ema_ = current_price;
            slow_ema_ = current_price;
            prev_price_ = current_price;
            is_initialized_ = true;
        } else {
            fast_ema_ = fast_alpha_ * current_price + (1.0 - fast_alpha_) * fast_ema_;
            med_ema_ = med_alpha_ * current_price + (1.0 - med_alpha_) * med_ema_;
            slow_ema_ = slow_alpha_ * current_price + (1.0 - slow_alpha_) * slow_ema_;
            if (prev_price_ > 0.0) {
                const double ret = (current_price - prev_price_) / prev_price_;
                vol_acc_.update(ret);
            }
            prev_price_ = current_price;
        }

        const double trend_spread = (fast_ema_ - slow_ema_) / current_price;
        const double trend_bps = trend_spread * 10000.0;

        sig.fast_ema = fast_ema_;
        sig.med_ema = med_ema_;
        sig.slow_ema = slow_ema_;
        sig.realized_vol = vol_acc_.stdev();
        sig.raw_trend_spread = trend_spread;
        sig.trend_spread_bps = trend_bps;

        // Actionable signal with deadband filter to eliminate noise
        if (trend_spread > 0.00020) {
            sig.composite_signal = 1.0;
        } else if (trend_spread < -0.00020) {
            sig.composite_signal = -1.0;
        } else {
            sig.composite_signal = 0.0;
        }

        sig.is_valid = true;
        assert(sig.composite_signal >= -1.0 && sig.composite_signal <= 1.0);
        assert(sig.is_valid);
        return sig;
    }

private:
    double fast_alpha_{0.005};
    double med_alpha_{0.001};
    double slow_alpha_{0.0005};
    double fast_ema_{0.0};
    double med_ema_{0.0};
    double slow_ema_{0.0};
    double prev_price_{0.0};
    WelfordAccumulator vol_acc_{};
    bool is_initialized_{false};
};

} // namespace alpha
