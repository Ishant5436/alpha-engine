#include <cassert>
#include <iostream>
#include "market_data.hpp"

void test_ring_buffer_push_and_vwap() {
    alpha::MarketDataRingBuffer<16> rb;
    assert(rb.empty());
    assert(rb.size() == 0);

    alpha::Tick t1{1000, 100.0, 100.2, 10.0, 10.0, 100.1, 5.0};
    assert(rb.push(t1));
    assert(!rb.empty());
    assert(rb.size() == 1);
    assert(rb.latest().last_price == 100.1);
    assert(rb.vwap() == 100.1);

    alpha::Tick t2{2000, 101.0, 101.2, 10.0, 10.0, 101.1, 15.0};
    assert(rb.push(t2));
    assert(rb.size() == 2);
    // VWAP: (100.1*5 + 101.1*15) / 20 = (500.5 + 1516.5) / 20 = 2017.0 / 20 = 100.85
    assert(rb.vwap() >= 100.84 && rb.vwap() <= 100.86);

    std::cout << "  [PASS] test_ring_buffer_push_and_vwap\n";
}

void test_ring_buffer_overflow_and_circularity() {
    alpha::MarketDataRingBuffer<4> rb;
    for (int i = 1; i <= 10; ++i) {
        alpha::Tick t{static_cast<uint64_t>(i * 1000), 100.0 + i, 100.2 + i, 1.0, 1.0, 100.0 + i, 1.0};
        assert(rb.push(t));
    }
    assert(rb.size() == 4);
    assert(rb.latest().last_price == 110.0);
    assert(rb[0].last_price == 110.0);
    assert(rb[1].last_price == 109.0);
    assert(rb[2].last_price == 108.0);
    assert(rb[3].last_price == 107.0);

    std::cout << "  [PASS] test_ring_buffer_overflow_and_circularity\n";
}
