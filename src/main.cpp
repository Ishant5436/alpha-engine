#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cassert>
#include "types.hpp"
#include "market_data.hpp"
#include "alpha_engine.hpp"
#include "risk_manager.hpp"
#include "backtester.hpp"

int main(int argc, char* argv[]) {
    const std::string filename = (argc > 1) ? argv[1] : "data/real_btc_ticks.bin";

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open tick data file: " << filename << "\n";
        return 1;
    }

    std::vector<alpha::Tick> ticks;
    constexpr std::size_t MAX_TICKS = 10000000;  // 10M tick safety cap (~560 MB)
    ticks.reserve(100000);

    alpha::Tick t{};
    while (ticks.size() < MAX_TICKS && file.read(reinterpret_cast<char*>(&t), sizeof(alpha::Tick))) {
        ticks.push_back(t);
    }
    file.close();

    std::cout << "Loaded " << ticks.size() << " market ticks from '" << filename << "'\n";
    if (ticks.empty()) {
        std::cout << "Warning: 0 market ticks loaded from '" << filename << "' - backtest skipped.\n";
        return 0;
    }

    alpha::Backtester backtester(
        10000.0,  // Initial Capital ($10,000)
        0.0392,   // Fast EMA alpha (50 ticks)
        0.00797,  // Medium EMA alpha (250 ticks)
        0.00160,  // Slow EMA alpha (1250 ticks)
        0.04,     // 4.0% Max Drawdown Kill-Switch
        1.0       // 1.0x Safe Leverage
    );

    const auto metrics = backtester.run(ticks.data(), ticks.size());

    // Print Performance Matrix
    std::cout << "\n=============================================================\n";
    std::cout << "                 STRATEGY PERFORMANCE REPORT\n";
    std::cout << "=============================================================\n";
    std::cout << "  Processed Ticks          : " << metrics.processed_ticks << "\n";
    std::cout << "  Throughput (Ticks/sec)   : " << static_cast<uint64_t>(metrics.ticks_per_second) << "\n";
    std::cout << "  Total Return (%)         : " << std::fixed << std::setprecision(2) << metrics.total_return_pct << "%\n";
    std::cout << "  Raw Per-Era Sharpe (μ/σ) : " << std::setprecision(4) << metrics.raw_per_era_sharpe << "\n";
    std::cout << "  Annualized Sharpe Ratio  : " << std::setprecision(4) << metrics.annualized_sharpe << "\n";
    std::cout << "  Maximum Drawdown (%)     : " << std::setprecision(2) << metrics.max_drawdown_pct << "%\n";
    std::cout << "  Total Executed Trades    : " << metrics.total_trades << "\n";
    std::cout << "  Win Rate (%)             : " << std::setprecision(2) << metrics.win_rate_pct << "%\n";
    std::cout << "  Profit Factor            : " << std::setprecision(2) << metrics.profit_factor << "\n";
    std::cout << "  Exchange Taker Fee Rate  : 4.0 bps per fill\n";
    std::cout << "=============================================================\n";

    // Write metrics.json for programmatic verification
    std::ofstream json_out("metrics.json");
    if (json_out.is_open()) {
        json_out << "{\n";
        json_out << "  \"processed_ticks\": " << metrics.processed_ticks << ",\n";
        json_out << "  \"ticks_per_second\": " << static_cast<uint64_t>(metrics.ticks_per_second) << ",\n";
        json_out << "  \"total_return_pct\": " << metrics.total_return_pct << ",\n";
        json_out << "  \"raw_per_era_sharpe\": " << metrics.raw_per_era_sharpe << ",\n";
        json_out << "  \"annualized_sharpe\": " << metrics.annualized_sharpe << ",\n";
        json_out << "  \"max_drawdown_pct\": " << metrics.max_drawdown_pct << ",\n";
        json_out << "  \"total_trades\": " << metrics.total_trades << ",\n";
        json_out << "  \"win_rate_pct\": " << metrics.win_rate_pct << ",\n";
        json_out << "  \"profit_factor\": " << metrics.profit_factor << ",\n";
        json_out << "  \"taker_fee_bps\": 4.0\n";
        json_out << "}\n";
        json_out.close();
    }

    return 0;
}
