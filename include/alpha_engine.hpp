#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>
#include "types.hpp"
#include "market_data.hpp"

namespace alpha {

constexpr std::size_t ALPHA_WARMUP_TICKS = 100;
constexpr double VOLATILITY_EXPANSION_THRESHOLD = 0.00015; // 1.5 bps vol floor to trade

struct AlphaSignal {
    double fast_ema{0.0};
    double med_ema{0.0};
    double slow_ema{0.0};
    double realized_vol{0.0};
    double trend_spread_bps{0.0};
    double composite_signal{0.0}; // 1.0 = Bull, -1.0 = Bear, 0.0 = Flat
    bool is_valid{false};
};

class MultiFactorAlphaEngine {
public:
    constexpr MultiFactorAlphaEngine(double fast_alpha = 0.0392, double med_alpha = 0.00797, double slow_alpha = 0.00160) noexcept
        : fast_alpha_(fast_alpha), med_alpha_(med_alpha), slow_alpha_(slow_alpha),
          fast_ema_(0.0), med_ema_(0.0), slow_ema_(0.0), is_initialized_(false) {
        assert(fast_alpha_ > med_alpha_);
        assert(med_alpha_ > slow_alpha_);
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
            is_initialized_ = true;
        } else {
            fast_ema_ = fast_alpha_ * current_price + (1.0 - fast_alpha_) * fast_ema_;
            med_ema_ = med_alpha_ * current_price + (1.0 - med_alpha_) * med_ema_;
            slow_ema_ = slow_alpha_ * current_price + (1.0 - slow_alpha_) * slow_ema_;
        }

        const double real_vol = calculate_realized_volatility(ring, 50);
        const double trend_spread = (fast_ema_ - slow_ema_) / current_price;
        const double trend_bps = trend_spread * 10000.0;

        sig.fast_ema = fast_ema_;
        sig.med_ema = med_ema_;
        sig.slow_ema = slow_ema_;
        sig.realized_vol = real_vol;
        sig.trend_spread_bps = trend_bps;

        // Triple-Timeframe Alignment + Volatility Expansion Gating
        const bool bull_aligned = (fast_ema_ > med_ema_) && (med_ema_ > slow_ema_);
        const bool bear_aligned = (fast_ema_ < med_ema_) && (med_ema_ < slow_ema_);

        if (real_vol >= VOLATILITY_EXPANSION_THRESHOLD) {
            if (bull_aligned && trend_bps > 3.0) {
                sig.composite_signal = 1.0; // Confirmed Bullish Macro Trend
            } else if (bear_aligned && trend_bps < -3.0) {
                sig.composite_signal = -1.0; // Confirmed Bearish Macro Trend
            } else {
                sig.composite_signal = 0.0; // Neutral
            }
        } else {
            sig.composite_signal = 0.0; // Chop Filter: Zero trades in deadbands
        }

        sig.is_valid = true;
        assert(sig.composite_signal >= -1.0 && sig.composite_signal <= 1.0);
        assert(sig.is_valid);
        return sig;
    }

private:
    double calculate_realized_volatility(const MarketDataRingBuffer<MAX_RING_CAPACITY>& ring, std::size_t window) const noexcept {
        assert(window > 0);
        assert(ring.size() >= window);
        const std::size_t limit = std::min(window, ring.size());
        double sum_ret_sq = 0.0;

        for (std::size_t i = 0; i < limit - 1; ++i) {
            const double p1 = ring[i].last_price;
            const double p0 = ring[i + 1].last_price;
            if (p0 > 0.0) {
                const double ret = (p1 - p0) / p0;
                sum_ret_sq += ret * ret;
            }
        }

        const double vol = std::sqrt(sum_ret_sq / static_cast<double>(limit));
        assert(vol >= 0.0);
        return vol;
    }

    double fast_alpha_{0.0392};
    double med_alpha_{0.00797};
    double slow_alpha_{0.00160};
    double fast_ema_{0.0};
    double med_ema_{0.0};
    double slow_ema_{0.0};
    bool is_initialized_{false};
};

} // namespace alpha
