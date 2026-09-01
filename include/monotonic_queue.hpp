#pragma once

#include <array>
#include <cstddef>
#include <cassert>
#include <functional>

namespace alpha {

/**
 * Monotonic Queue (Sliding-Window Extrema Engine)
 * Invariant: Maintains strictly ordered extrema over a rolling window.
 * Complexity: O(1) amortized push, O(1) amortized expiration, O(1) query.
 * Memory: Zero heap allocation, 64-byte L1 cache-line aligned array.
 */
template <typename T, std::size_t Capacity = 2048, typename Compare = std::greater<T>>
class MonotonicQueue {
public:
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    constexpr MonotonicQueue() noexcept : head_(0), tail_(0), size_(0) {
        assert(Capacity > 0);
        assert(size_ == 0);
    }

    void push(const T& val, uint64_t index) noexcept {
        while (size_ > 0) {
            const std::size_t last_idx = (tail_ - 1) & (Capacity - 1);
            if (!Compare{}(val, data_[last_idx].value)) {
                break;
            }
            tail_ = (tail_ - 1) & (Capacity - 1);
            --size_;
        }
        const std::size_t ins_idx = tail_ & (Capacity - 1);
        data_[ins_idx] = Entry{val, index};
        tail_ = (tail_ + 1) & (Capacity - 1);
        ++size_;
        assert(size_ <= Capacity);
    }

    void pop_expired(uint64_t min_valid_index) noexcept {
        while (size_ > 0) {
            const std::size_t first_idx = head_ & (Capacity - 1);
            if (data_[first_idx].index >= min_valid_index) {
                break;
            }
            head_ = (head_ + 1) & (Capacity - 1);
            --size_;
        }
        assert(size_ <= Capacity);
    }

    [[nodiscard]] const T& top() const noexcept {
        assert(size_ > 0);
        const std::size_t first_idx = head_ & (Capacity - 1);
        return data_[first_idx].value;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

private:
    struct Entry {
        T value{};
        uint64_t index{0};
    };

    alignas(64) std::array<Entry, Capacity> data_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
};

} // namespace alpha
