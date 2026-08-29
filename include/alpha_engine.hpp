#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>
#include "types.hpp"
#include "market_data.hpp"

namespace alpha {

constexpr std::size_t ALPHA_WARMUP_TICKS = 30;

struct AlphaSignal {
    double momentum_score{0.0};       // [-1.0, 1.0]
    double mean_reversion_score{0.0}; // [-1.0, 1.0]
    double volatility_regime{0.0};    // Realized volatility
    double composite_signal{0.0};     // [-1.0, 1.0]
    bool is_valid{false};
};

class MultiFactorAlphaEngine {
public:
    constexpr MultiFactorAlphaEngine(double fast_alpha = 0.08, double slow_alpha = 0.015, double vol_threshold = 0.0008) noexcept
        : fast_alpha_(fast_alpha), slow_alpha_(slow_alpha), vol_threshold_(vol_threshold),
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

        const double real_vol = calculate_realized_volatility(ring, 20);
        const double denom = current_price * std::max(real_vol, 0.00005);

        const double mom_diff = (fast_ema_ - slow_ema_);
        const double mom_raw = mom_diff / denom;
        sig.momentum_score = std::clamp(mom_raw * 0.8, -1.0, 1.0);

        const double vwap_val = ring.vwap();
        const double mr_diff = (vwap_val - current_price);
        const double mr_raw = mr_diff / denom;
        sig.mean_reversion_score = std::clamp(mr_raw * 0.6, -1.0, 1.0);

        sig.volatility_regime = real_vol;

        // Dynamic Regime-Switching
        if (real_vol > vol_threshold_ || std::abs(sig.momentum_score) > 0.3) {
            // Trend Regime: Follow Momentum Breakout
            sig.composite_signal = (std::abs(sig.momentum_score) > 0.15) ? sig.momentum_score : 0.0;
        } else {
            // Range Regime: Mean Reversion to VWAP
            sig.composite_signal = (std::abs(sig.mean_reversion_score) > 0.25) ? sig.mean_reversion_score : 0.0;
        }

        sig.composite_signal = std::clamp(sig.composite_signal, -1.0, 1.0);
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

    double fast_alpha_{0.08};
    double slow_alpha_{0.015};
    double vol_threshold_{0.0008};
    double fast_ema_{0.0};
    double slow_ema_{0.0};
    bool is_initialized_{false};
};

} // namespace alpha
