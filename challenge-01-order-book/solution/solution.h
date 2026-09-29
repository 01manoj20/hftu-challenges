#pragma once
// Challenge 01: Order Book
// Edit this file and solution.cpp to implement your solution.

#include <cstdint>
#include <array>
#include <algorithm>


namespace hftu {

class OrderBook {

public:

    OrderBook();
    ~OrderBook();

    // levels_/bitmasks_ are raw mmap'd (Linux) allocations now, not
    // std::vector — copying/assigning an OrderBook would double-free them,
    // and nothing in the benchmark ever needs to copy one.
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

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

    // price is uniform-random over [1, 1,000,000], so levels_/bitmasks_
    // (~8MB each) get touched on effectively random 4KB pages — on Linux
    // that's ~2000 distinct pages fighting over a TLB with far fewer
    // entries. alloc_maybe_huge backs these with an explicit MAP_HUGETLB
    // reservation (2MB pages -> ~4 pages instead of ~2000), not a THP
    // hint, since THP may be disabled; falls back to a plain zeroed array
    // on non-Linux platforms, where there's no equivalent userspace knob.
    template <typename T>
    static T* alloc_maybe_huge(size_t count);
    template <typename T>
    static void free_maybe_huge(T* ptr, size_t count);

    // array<T*,2> instead of vector<vector<>>: side indexing becomes
    // fixed-offset (no extra heap-pointer load) since the outer container is
    // inline, not itself heap-allocated.
    std::array<int64_t*, 2> levels_;
    std::array<uint64_t*, 2> bitmasks_;
    // Also random-access by id (whichever active order a cancel happens to
    // pick), so it gets the same huge-page treatment as levels_/bitmasks_,
    // just over a smaller actually-touched footprint (~480KB for this
    // benchmark's ~60k ids vs. the full 8MB table).
    Order* orders_array_;

    int64_t best_[2];

};

} // namespace hftu
