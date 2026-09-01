#pragma once

#include <cmath>
#include <cstddef>
#include <cassert>

namespace alpha {

/**
 * Welford Statistical Accumulator
 * Invariant: Single-pass, numerically stable rolling mean and variance.
 * Complexity: O(1) time per sample, O(1) space.
 * Guarantee: Zero catastrophic cancellation error in floating-point moments.
 */
class WelfordAccumulator {
public:
    constexpr WelfordAccumulator() noexcept : count_(0), mean_(0.0), m2_(0.0) {
        assert(count_ == 0);
        assert(mean_ == 0.0);
    }

    constexpr void update(double x) noexcept {
        ++count_;
        const double delta = x - mean_;
        mean_ += delta / static_cast<double>(count_);
        const double delta2 = x - mean_;
        m2_ += delta * delta2;
        assert(count_ > 0);
        assert(m2_ >= 0.0);
    }

    [[nodiscard]] constexpr double mean() const noexcept {
        return mean_;
    }

    [[nodiscard]] constexpr double variance() const noexcept {
        return (count_ > 1) ? (m2_ / static_cast<double>(count_ - 1)) : 0.0;
    }

    [[nodiscard]] double stdev() const noexcept {
        const double var = variance();
        assert(var >= 0.0);
        return std::sqrt(var);
    }

    [[nodiscard]] constexpr std::size_t count() const noexcept {
        return count_;
    }

    constexpr void reset() noexcept {
        count_ = 0;
        mean_ = 0.0;
        m2_ = 0.0;
    }

private:
    std::size_t count_{0};
    double mean_{0.0};
    double m2_{0.0};
};

} // namespace alpha
