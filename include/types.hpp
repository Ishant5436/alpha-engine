#pragma once

#include <cstdint>
#include <cstddef>
#include <cassert>

namespace alpha {

// NASA Power of 10 Compile-Time Constants
constexpr std::size_t MAX_RING_CAPACITY = 2048;
constexpr std::size_t MAX_ORDER_ITERATIONS = 10;
constexpr double MAX_DRAWDOWN_THRESHOLD = 0.04;  // 4.0% hard stop
constexpr double MAX_LEVERAGE_CAP = 3.0;         // 3.0x max leverage
constexpr double TRADING_FEE_RATE = 0.0004;      // 4 bps taker fee

enum class OrderSide : uint8_t {
    NONE = 0,
    BUY = 1,
    SELL = 2
};

struct Tick {
    uint64_t timestamp_ns{0};
    double bid_price{0.0};
    double ask_price{0.0};
    double bid_size{0.0};
    double ask_size{0.0};
    double last_price{0.0};
    double volume{0.0};

    [[nodiscard]] constexpr double mid_price() const noexcept {
        return 0.5 * (bid_price + ask_price);
    }

    [[nodiscard]] constexpr double spread() const noexcept {
        return ask_price - bid_price;
    }
};

struct Position {
    double size{0.0};          // positive = long, negative = short
    double entry_price{0.0};
    double realized_pnl{0.0};
    double unrealized_pnl{0.0};
    double peak_equity{10000.0};
    double current_equity{10000.0};
    bool is_liquidated{false};
};

struct TradeRecord {
    uint64_t timestamp_ns{0};
    OrderSide side{OrderSide::NONE};
    double price{0.0};
    double size{0.0};
    double realized_pnl{0.0};
    double fee_paid{0.0};
};

struct PerformanceMetrics {
    double total_return_pct{0.0};
    double raw_per_era_sharpe{0.0};
    double annualized_sharpe{0.0};
    double max_drawdown_pct{0.0};
    double win_rate_pct{0.0};
    double profit_factor{0.0};
    std::size_t total_trades{0};
    std::size_t winning_trades{0};
    std::size_t losing_trades{0};
    uint64_t processed_ticks{0};
    double ticks_per_second{0.0};
};

} // namespace alpha
