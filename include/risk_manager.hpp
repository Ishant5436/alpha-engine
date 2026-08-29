#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>
#include "types.hpp"
#include "alpha_engine.hpp"

namespace alpha {

class RiskManager {
public:
    constexpr explicit RiskManager(double initial_capital = 10000.0,
                                   double max_drawdown = MAX_DRAWDOWN_THRESHOLD,
                                   double max_leverage = MAX_LEVERAGE_CAP) noexcept
        : initial_capital_(initial_capital), max_drawdown_(max_drawdown), max_leverage_(max_leverage) {
        assert(initial_capital_ > 0.0);
        assert(max_drawdown_ > 0.0 && max_drawdown_ < 1.0);
        assert(max_leverage_ >= 1.0 && max_leverage_ <= 10.0);
        position_.peak_equity = initial_capital_;
        position_.current_equity = initial_capital_;
    }

    [[nodiscard]] double calculate_target_position(const AlphaSignal& signal, double current_price) noexcept {
        assert(current_price > 0.0);
        assert(position_.current_equity >= 0.0);

        if (position_.is_liquidated || !signal.is_valid) {
            return 0.0;
        }

        // Bounded position sizing: base on equity * leverage * signal conviction
        const double max_allowed_notional = position_.current_equity * max_leverage_;
        const double raw_target_notional = max_allowed_notional * signal.composite_signal;
        const double target_units = raw_target_notional / current_price;

        assert(std::abs(target_units * current_price) <= max_allowed_notional + 1e-4);
        return target_units;
    }

    bool update_pnl(double current_price) noexcept {
        assert(current_price > 0.0);
        assert(position_.peak_equity > 0.0);

        if (position_.size != 0.0) {
            position_.unrealized_pnl = position_.size * (current_price - position_.entry_price);
        } else {
            position_.unrealized_pnl = 0.0;
        }

        position_.current_equity = initial_capital_ + position_.realized_pnl + position_.unrealized_pnl;
        if (position_.current_equity > position_.peak_equity) {
            position_.peak_equity = position_.current_equity;
        }

        // Hard Circuit Breaker / Drawdown Kill-Switch
        const double current_drawdown = (position_.peak_equity - position_.current_equity) / position_.peak_equity;
        if (current_drawdown >= max_drawdown_ && !position_.is_liquidated) {
            position_.is_liquidated = true;
            position_.realized_pnl += position_.unrealized_pnl;
            position_.unrealized_pnl = 0.0;
            position_.size = 0.0; // Emergency Flatten
            return false;         // Triggered Kill-Switch
        }

        assert(position_.current_equity >= 0.0);
        return !position_.is_liquidated;
    }

    void execute_fill(OrderSide side, double fill_price, double fill_size) noexcept {
        assert(fill_price > 0.0);
        assert(fill_size > 0.0);
        if (position_.is_liquidated) {
            return;
        }

        const double signed_size = (side == OrderSide::BUY) ? fill_size : -fill_size;
        const double new_size = position_.size + signed_size;

        if (position_.size == 0.0) {
            position_.entry_price = fill_price;
        } else if ((position_.size > 0.0 && signed_size > 0.0) || (position_.size < 0.0 && signed_size < 0.0)) {
            // Increase existing position (weighted avg entry)
            position_.entry_price = (position_.entry_price * std::abs(position_.size) + fill_price * fill_size) / std::abs(new_size);
        } else {
            // Closing or reducing position
            const double closed_size = std::min(std::abs(position_.size), fill_size);
            const double pnl_per_unit = (position_.size > 0.0) ? (fill_price - position_.entry_price) : (position_.entry_price - fill_price);
            position_.realized_pnl += closed_size * pnl_per_unit;
        }

        position_.size = new_size;
        assert(std::isfinite(position_.realized_pnl));
    }

    [[nodiscard]] const Position& position() const noexcept {
        return position_;
    }

private:
    double initial_capital_{10000.0};
    double max_drawdown_{MAX_DRAWDOWN_THRESHOLD};
    double max_leverage_{MAX_LEVERAGE_CAP};
    Position position_{};
};

} // namespace alpha
