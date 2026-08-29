#pragma once

#include <array>
#include <cstddef>
#include <cassert>
#include "types.hpp"

namespace alpha {

template <std::size_t N = MAX_RING_CAPACITY>
class MarketDataRingBuffer {
public:
    static_assert(N > 0 && (N & (N - 1)) == 0, "Capacity N must be a power of 2");

    constexpr MarketDataRingBuffer() noexcept : head_(0), count_(0), cumulative_volume_(0.0), cumulative_pv_(0.0) {
        assert(N <= MAX_RING_CAPACITY);
        assert(count_ == 0);
    }

    bool push(const Tick& tick) noexcept {
        assert(tick.bid_price > 0.0);
        assert(tick.ask_price >= tick.bid_price);

        const std::size_t idx = head_ & (N - 1);
        if (count_ >= N) {
            const Tick& old_tick = buffer_[idx];
            cumulative_volume_ -= old_tick.volume;
            cumulative_pv_ -= old_tick.last_price * old_tick.volume;
        } else {
            ++count_;
        }

        buffer_[idx] = tick;
        cumulative_volume_ += tick.volume;
        cumulative_pv_ += tick.last_price * tick.volume;
        ++head_;

        assert(count_ <= N);
        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        assert(count_ <= N);
        return count_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return count_ == 0;
    }

    [[nodiscard]] const Tick& latest() const noexcept {
        assert(count_ > 0);
        const std::size_t idx = (head_ - 1) & (N - 1);
        return buffer_[idx];
    }

    [[nodiscard]] const Tick& operator[](std::size_t offset_from_latest) const noexcept {
        assert(offset_from_latest < count_);
        const std::size_t idx = (head_ - 1 - offset_from_latest) & (N - 1);
        return buffer_[idx];
    }

    [[nodiscard]] double vwap() const noexcept {
        if (cumulative_volume_ <= 0.0) {
            return 0.0;
        }
        assert(cumulative_volume_ > 0.0);
        return cumulative_pv_ / cumulative_volume_;
    }

private:
    std::array<Tick, N> buffer_{};
    std::size_t head_{0};
    std::size_t count_{0};
    double cumulative_volume_{0.0};
    double cumulative_pv_{0.0};
};

} // namespace alpha
