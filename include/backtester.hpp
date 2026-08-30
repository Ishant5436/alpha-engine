#pragma once

#include <vector>
#include <cmath>
#include <chrono>
#include <cassert>
#include "types.hpp"
#include "market_data.hpp"
#include "alpha_engine.hpp"
#include "risk_manager.hpp"

namespace alpha {

class Backtester {
public:
    constexpr explicit Backtester(double initial_capital = 10000.0,
                                  double fast_alpha = 0.005,
                                  double slow_alpha = 0.0005,
                                  double max_drawdown = MAX_DRAWDOWN_THRESHOLD,
                                  double max_leverage = 1.0) noexcept
        : ring_(), alpha_engine_(fast_alpha, slow_alpha),
          risk_manager_(initial_capital, max_drawdown, max_leverage),
          initial_capital_(initial_capital) {
        assert(initial_capital_ > 0.0);
        assert(max_drawdown > 0.0);
    }

    PerformanceMetrics run(const Tick* ticks, std::size_t num_ticks) noexcept {
        assert(ticks != nullptr);
        assert(num_ticks > 0);

        const auto start_time = std::chrono::high_resolution_clock::now();
        std::size_t winning_trades = 0;
        std::size_t losing_trades = 0;
        double gross_profit = 0.0;
        double gross_loss = 0.0;
        double prev_equity = initial_capital_;
        double max_observed_dd_pct = 0.0;

        double sum_returns = 0.0;
        double sum_sq_returns = 0.0;
        std::size_t return_count = 0;

        for (std::size_t i = 0; i < num_ticks; ++i) {
            const Tick& tick = ticks[i];
            ring_.push(tick);
            risk_manager_.update_pnl(tick.mid_price());

            const double current_eq = risk_manager_.position().current_equity;
            const double peak_eq = risk_manager_.position().peak_equity;
            const double tick_dd_pct = 100.0 * (peak_eq - current_eq) / std::max(peak_eq, 1.0);
            if (tick_dd_pct > max_observed_dd_pct) {
                max_observed_dd_pct = tick_dd_pct;
            }

            const AlphaSignal signal = alpha_engine_.update(ring_);
            if (signal.is_valid && !risk_manager_.position().is_liquidated) {
                const double current_size = risk_manager_.position().size;
                const double mid = tick.mid_price();
                const double unit_size = (current_eq * 1.0) / mid; // 1.0x safe leverage
                double target_size = current_size;

                if (current_size == 0.0) {
                    if (signal.composite_signal > 0.5) {
                        target_size = unit_size;
                    } else if (signal.composite_signal < -0.5) {
                        target_size = -unit_size;
                    }
                } else {
                    const double unpnl = risk_manager_.position().unrealized_pnl;
                    // Stop loss or trend reversal exit
                    if (unpnl < -35.0 ||
                        (current_size > 0.0 && signal.raw_trend_spread < 0.0) ||
                        (current_size < 0.0 && signal.raw_trend_spread > 0.0)) {
                        target_size = 0.0;
                    }
                }

                const double delta_size = target_size - current_size;
                if (std::abs(delta_size) > 0.001) {
                    const OrderSide side = (delta_size > 0.0) ? OrderSide::BUY : OrderSide::SELL;
                    const double fill_price = (side == OrderSide::BUY) ? tick.ask_price : tick.bid_price;
                    const double trade_size = std::abs(delta_size);

                    const double prev_realized = risk_manager_.position().realized_pnl;
                    risk_manager_.execute_fill(side, fill_price, trade_size);
                    const double trade_pnl = risk_manager_.position().realized_pnl - prev_realized;

                    if (target_size == 0.0) {
                        if (trade_pnl > 0.0) {
                            ++winning_trades;
                            gross_profit += trade_pnl;
                        } else if (trade_pnl < 0.0) {
                            ++losing_trades;
                            gross_loss += std::abs(trade_pnl);
                        }
                    }
                }
            }

            // Sample returns every 200 ticks
            if (i % 200 == 0 && i > 0) {
                const double ret = (current_eq - prev_equity) / std::max(prev_equity, 1.0);
                sum_returns += ret;
                sum_sq_returns += ret * ret;
                ++return_count;
                prev_equity = current_eq;
            }
        }

        const auto end_time = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> elapsed = end_time - start_time;

        PerformanceMetrics metrics{};
        metrics.processed_ticks = num_ticks;
        metrics.ticks_per_second = (elapsed.count() > 0.0) ? (static_cast<double>(num_ticks) / elapsed.count()) : 0.0;
        metrics.total_trades = winning_trades + losing_trades;
        metrics.winning_trades = winning_trades;
        metrics.losing_trades = losing_trades;
        metrics.win_rate_pct = (metrics.total_trades > 0) ? (100.0 * static_cast<double>(winning_trades) / static_cast<double>(metrics.total_trades)) : 0.0;
        metrics.profit_factor = (gross_loss > 0.0) ? (gross_profit / gross_loss) : ((gross_profit > 0.0) ? 99.0 : 1.0);

        const double final_equity = risk_manager_.position().current_equity;
        metrics.total_return_pct = 100.0 * (final_equity - initial_capital_) / initial_capital_;
        metrics.max_drawdown_pct = max_observed_dd_pct;

        if (return_count > 1) {
            const double mean_ret = sum_returns / static_cast<double>(return_count);
            const double var_ret = (sum_sq_returns / static_cast<double>(return_count)) - (mean_ret * mean_ret);
            const double std_ret = std::sqrt(std::max(var_ret, 1e-10));
            metrics.raw_per_era_sharpe = mean_ret / std_ret;
            metrics.annualized_sharpe = metrics.raw_per_era_sharpe * std::sqrt(252.0 * 24.0 * 6.0);
        }

        assert(metrics.processed_ticks == num_ticks);
        assert(std::isfinite(metrics.total_return_pct));
        return metrics;
    }

private:
    MarketDataRingBuffer<MAX_RING_CAPACITY> ring_{};
    MultiFactorAlphaEngine alpha_engine_{};
    RiskManager risk_manager_{};
    double initial_capital_{10000.0};
};

} // namespace alpha
