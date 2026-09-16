#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include "types.hpp"
#include "market_data.hpp"
#include "alpha_engine.hpp"
#include "risk_manager.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < sizeof(alpha::Tick)) {
        return 0;
    }

    alpha::MarketDataRingBuffer<128> ring;
    alpha::MultiFactorAlphaEngine engine(0.08, 0.02, 0.005);
    alpha::RiskManager risk_mgr(10000.0);

    size_t offset = 0;
    size_t count = 0;
    while (offset + sizeof(alpha::Tick) <= size && count < 64) {
        alpha::Tick tick;
        std::memcpy(&tick, data + offset, sizeof(alpha::Tick));
        offset += sizeof(alpha::Tick);
        count++;

        // Enforce invariants expected by ring buffer preconditions
        if (std::isfinite(tick.bid_price) && tick.bid_price > 0.01 && tick.bid_price < 1e7 &&
            std::isfinite(tick.ask_price) && tick.ask_price >= tick.bid_price && tick.ask_price < 1e7 &&
            std::isfinite(tick.volume) && tick.volume >= 0.0 && tick.volume < 1e7 &&
            std::isfinite(tick.last_price) && tick.last_price > 0.01 && tick.last_price < 1e7) {
            
            ring.push(tick);
            (void)ring.vwap();
            (void)ring.latest();

            // Run risk updates
            (void)risk_mgr.update_pnl(tick.last_price);
        }
    }

    return 0;
}
