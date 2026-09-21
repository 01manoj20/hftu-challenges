#pragma once
// Challenge 01: Order Book
// Edit this file and solution.cpp to implement your solution.

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>


namespace hftu {

class OrderBook {

public:

    OrderBook();

    // Add a new order. side: 0=buy(bid), 1=sell(ask)
    void add_order(uint64_t id, int side, int64_t price, int64_t quantity);

    // Cancel an order by ID. No-op if ID doesn't exist.
    void cancel_order(uint64_t id);

    // Return highest bid price, or 0 if no bids.
    int64_t best_bid() const;

    // Return lowest ask price, or 0 if no asks.
    int64_t best_ask() const;

private:
    // Packed to 8 bytes (vs. 24 for the {int, int64_t, int64_t} layout) so the
    // id-indexed order table is 3x denser in cache — this array is touched on
    // every add/cancel via a random id, so its cache footprint dominates.
    struct Order {
        int32_t price;      // range [1, 1'000'000] fits comfortably
        uint16_t quantity;  // range [1, 10'000] fits comfortably
        uint8_t side;
        uint8_t active;     // explicit flag: quantity==0 is a valid input we must not alias
    };

    static constexpr int64_t kMaxPrice = 1'000'000;
    static constexpr size_t kLevelsSize = kMaxPrice + 5;
    static constexpr size_t kChunks = kLevelsSize / 64 + 1;
    // Ids are unbounded uint64_t per the spec, but active orders are capped at
    // 1,000,000; size the direct-index table to that bound (the previous
    // 100,010 only happened to survive the shipped 100k-op benchmark and
    // heap-overflows past ~100k distinct ids — verified with ASan).
    static constexpr size_t kMaxOrders = 1'000'005;

    // array<vector,2> instead of vector<vector<>>: side indexing becomes
    // fixed-offset (no extra heap-pointer load) since the outer container is
    // inline, not itself heap-allocated.
    std::array<std::vector<int64_t>, 2> levels_;
    std::array<std::vector<uint64_t>, 2> bitmasks_;
    std::vector<Order> orders_array_;

    int64_t current_best_bid_;
    int64_t current_best_ask_;


};

} // namespace hftu
