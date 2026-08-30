#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <chrono>
#include <cmath>
#include <vector>
#include <cassert>
#include "types.hpp"
#include "market_data.hpp"
#include "alpha_engine.hpp"
#include "risk_manager.hpp"

using namespace alpha;

struct PaperFill {
    std::string timestamp;
    std::string side;
    double price{0.0};
    double qty{0.0};
    double fee{0.0};
    double pnl{0.0};
};

int main(int argc, char* argv[]) {
    std::string symbol = "BTCUSDT";
    double initial_capital = 10000.0;
    if (argc > 1) symbol = argv[1];
    if (argc > 2) initial_capital = std::stod(argv[2]);

    MarketDataRingBuffer<MAX_RING_CAPACITY> ring;
    MultiFactorAlphaEngine engine(0.0392, 0.00797, 0.00160);
    RiskManager risk_mgr(initial_capital, MAX_DRAWDOWN_THRESHOLD, MAX_LEVERAGE_CAP, TRADING_FEE_RATE);

    std::uint64_t total_trades = 0;
    std::uint64_t winning_trades = 0;
    std::vector<PaperFill> fill_history;

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    std::uint64_t tick_count = 0;

    // Clear screen
    std::cout << "\033[2J\033[H";

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::uint64_t ts_ms = 0;
        double price = 0.0, qty = 0.0;
        std::string side_str;

        if (!(ss >> ts_ms >> price >> qty >> side_str)) {
            continue;
        }

        tick_count++;
        const double spread = std::max(0.01, price * 0.0001);
        const double bid = price - spread / 2.0;
        const double ask = price + spread / 2.0;

        Tick tick{
            .timestamp_ns = ts_ms * 1000000ULL,
            .bid_price = bid,
            .ask_price = ask,
            .bid_size = qty,
            .ask_size = qty,
            .last_price = price,
            .volume = qty
        };

        ring.push(tick);
        AlphaSignal sig = engine.update(ring);
        risk_mgr.update_pnl(price);

        const auto& pos = risk_mgr.position();

        // Check and execute paper fills
        if (sig.is_valid && !pos.is_liquidated) {
            const double target_size = risk_mgr.calculate_target_position(sig, price);
            const double size_diff = target_size - pos.size;

            if (std::abs(size_diff * price) >= 10.0) { // $10 min notional threshold
                const OrderSide side = (size_diff > 0) ? OrderSide::BUY : OrderSide::SELL;
                const double fill_price = (side == OrderSide::BUY) ? ask : bid;
                const double fill_qty = std::abs(size_diff);
                const double prev_realized = pos.realized_pnl;

                risk_mgr.execute_fill(side, fill_price, fill_qty);
                total_trades++;
                const double trade_pnl = pos.realized_pnl - prev_realized;
                if (trade_pnl > 0.0) winning_trades++;

                fill_history.push_back({
                    (side == OrderSide::BUY ? "BUY" : "SELL"),
                    (target_size > 0 ? "LONG" : (target_size < 0 ? "SHORT" : "FLAT")),
                    fill_price,
                    fill_qty,
                    fill_price * fill_qty * TRADING_FEE_RATE,
                    trade_pnl
                });
            }
        }

        // Render Live ANSI Terminal Dashboard (every 5 ticks)
        if (tick_count % 5 == 0) {
            std::cout << "\033[H"; // Move cursor to top-left
            std::cout << "\033[1;36m================================================================================\033[0m\n";
            std::cout << "\033[1;37m        ALPHAENGINE LIVE WEBSOCKET FORWARD PAPER TRADER (BINANCE SPOT)        \033[0m\n";
            std::cout << "\033[1;36m================================================================================\033[0m\n";
            
            std::cout << "\033[1;33m  Symbol\033[0m: " << std::left << std::setw(10) << symbol
                      << "\033[1;33m  Live Price\033[0m: $" << std::fixed << std::setprecision(2) << std::setw(10) << price
                      << "\033[1;33m  Ticks Processed\033[0m: " << tick_count << "\n";

            std::cout << "\033[1;33m  VWAP (Ring)\033[0m: $" << std::setw(10) << ring.vwap()
                      << "\033[1;33m  Parkinson Vol\033[0m: " << std::setprecision(1) << (sig.realized_vol * 10000.0) << " bps"
                      << " (" << ((sig.realized_vol * 10000.0) < 1.5 ? "\033[1;31mCHOP GATED\033[0m" : "\033[1;32mEXPANSION\033[0m") << ")\n";

            std::cout << "\033[1;33m  Fast EMA\033[0m: $" << std::setprecision(2) << sig.fast_ema
                      << "  \033[1;33mMed EMA\033[0m: $" << sig.med_ema
                      << "  \033[1;33mSlow EMA\033[0m: $" << sig.slow_ema << "\n";

            std::cout << "--------------------------------------------------------------------------------\n";
            std::cout << "\033[1;37m  POSITION STATE & ACCOUNTING\033[0m\n";
            std::cout << "  Position State  : " 
                      << (pos.size > 0.0001 ? "\033[1;32m[ LONG ]\033[0m" : (pos.size < -0.0001 ? "\033[1;31m[ SHORT ]\033[0m" : "\033[1;33m[ FLAT / GATED ]\033[0m"))
                      << (pos.is_liquidated ? " \033[1;41;37m[ CIRCUIT BREAKER HALTED ]\033[0m" : "") << "\n";
            std::cout << "  Position Units  : " << std::setprecision(4) << pos.size << " " << symbol 
                      << " (Entry: $" << std::setprecision(2) << pos.entry_price << ")\n";
            
            std::cout << "  Initial Capital : $" << std::setprecision(2) << initial_capital << "\n";
            std::cout << "  Total Equity    : $" << pos.current_equity << " (" << ((pos.current_equity >= initial_capital) ? "\033[1;32m+" : "\033[1;31m") 
                      << std::setprecision(2) << ((pos.current_equity - initial_capital) / initial_capital * 100.0) << "%\033[0m)\n";
            
            std::cout << "  Realized PnL    : $" << pos.realized_pnl << "   Unrealized PnL: $" << pos.unrealized_pnl << "\n";
            std::cout << "  Taker Fees Paid : $" << risk_mgr.cumulative_fees() << " (4.0 bps institutional fee)\n";
            std::cout << "  Total Trades    : " << total_trades 
                      << "   Win Rate: " << (total_trades > 0 ? (winning_trades * 100.0 / total_trades) : 0.0) << "%\n";

            std::cout << "--------------------------------------------------------------------------------\n";
            std::cout << "\033[1;37m  RECENT PAPER EXECUTION TAPE (LAST 3 FILLS)\033[0m\n";
            if (fill_history.empty()) {
                std::cout << "  No fills executed yet. Awaiting volatility breakout alignment...\n";
            } else {
                const std::size_t start_idx = fill_history.size() > 3 ? fill_history.size() - 3 : 0;
                for (std::size_t i = start_idx; i < fill_history.size(); ++i) {
                    const auto& f = fill_history[i];
                    std::cout << "  [" << f.side << "] " << std::setw(6) << f.timestamp 
                              << " @ $" << std::fixed << std::setprecision(2) << f.price 
                              << " | Qty: " << std::setprecision(4) << f.qty
                              << " | Fee: $" << std::setprecision(2) << f.fee;
                    if (f.pnl != 0.0) {
                        std::cout << " | PnL: " << (f.pnl > 0 ? "\033[1;32m+$" : "\033[1;31m-$") << std::abs(f.pnl) << "\033[0m";
                    }
                    std::cout << "\n";
                }
            }
            std::cout << "\033[1;36m================================================================================\033[0m\n";
            std::cout << "  Press Ctrl+C to stop live forward paper trader\n" << std::flush;
        }
    }

    return 0;
}
