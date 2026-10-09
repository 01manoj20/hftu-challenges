#pragma once
// Challenge 01: Order Book
// Edit this file and solution.cpp to implement your solution.

#include <cstdint>
#include <array>
#include <algorithm>
#include <unordered_map>


namespace hftu {

class OrderBook {

public:

    OrderBook();
    ~OrderBook();

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // Add a new order. side: 0=buy(bid), 1=sell(ask)
    void add_order(uint64_t id, int side, int64_t price, int64_t quantity);

    // Cancel an order by ID. No-op if ID doesn't exist.
    void cancel_order(uint64_t id);


    // Return highest bid price, or 0 if no bids.
    int64_t best_bid() const { return best_[0]; }

    // Return lowest ask price, or 0 if no asks.
    int64_t best_ask() const { return (best_[1] == INT64_MIN) ? 0 : -best_[1]; }

private:
    // What cancel_order needs to remember about an order, packed in 4 bytes:
    //   ref = (v << 1) | side
    //   v == 0                      no active order
    //   1 <= v <= kPriceTableSlots  the order's price lives in price slot v-1
    //   v >  kPriceTableSlots       the price is (v - kPriceTableSlots - 1) and
    //                               lives in price_overflow_ (see below)
    // In the common case the slot already holds the price, so the order
    // doesn't store it, and cancel jumps straight to its slot instead of
    // hashing and probing. Slots are claimed permanently and never move, so
    // the index stays valid.
    //
    // Assumes the published price range [1, 1'000'000] (bitmasks_ is sized
    // for it, and the packed value above relies on it).

    struct PriceSlot {
        int32_t price;
        int32_t count;  // number of active orders resting at this price
    };

    static constexpr int64_t kMaxPrice = 1'000'000;
    static constexpr size_t kLevelsSize = kMaxPrice + 5;
    static constexpr size_t kChunks = kLevelsSize / 64 + 1;

    // Order ids index orders_fast_ directly when id < kOrdersFastSlots (ids
    // are handed out sequentially, so this is the common case). Any larger id,
    // up to the full uint64_t range, is kept in orders_overflow_ instead.
    static constexpr size_t kOrdersFastSlots = 131'072;

    static constexpr size_t kPriceTableBits = 17;
    static constexpr size_t kPriceTableSlots = size_t(1) << kPriceTableBits;
    static constexpr size_t kPriceTableMask = kPriceTableSlots - 1;
    // A side stops handing out table slots once 75% are claimed; further
    // prices go to price_overflow_. Keeping the table from filling up also
    // guarantees every probe sequence reaches an empty slot.
    static constexpr uint32_t kPriceTableMaxClaimed = kPriceTableSlots / 4 * 3;
    static constexpr size_t kNoSlot = ~size_t(0);
    static constexpr uint64_t kHashMul = 0x9E3779B97F4A7C15ULL; // Fibonacci hashing

    static size_t hash_price(int64_t price) {
        return static_cast<size_t>((static_cast<uint64_t>(price) * kHashMul) >> (64 - kPriceTableBits));
    }

    // Returns the index in price_table_[side] of this price's slot, claiming
    // an empty one if the price isn't there yet. Returns kNoSlot if the price
    // isn't in the table and the table is already as full as we allow, in
    // which case the price is tracked in price_overflow_.
    size_t find_or_create_slot(int side, int64_t price);

    std::array<uint64_t*, 2> bitmasks_;
    std::array<PriceSlot*, 2> price_table_;
    std::array<uint32_t, 2> claimed_;                       // slots claimed per side
    std::array<std::unordered_map<int64_t, int32_t>, 2> price_overflow_;  // price -> order count

    uint32_t* orders_fast_;
    std::unordered_map<uint64_t, uint32_t> orders_overflow_;  // id -> ref, for id >= kOrdersFastSlots

    int64_t best_[2];

};

} // namespace hftu
