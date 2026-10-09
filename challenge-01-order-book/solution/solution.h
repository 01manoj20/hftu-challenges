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
    // What cancel_order needs to remember about an order, packed in 4 bytes
    // (stored in orders_fast_, indexed by order id):
    //   bit 0      side
    //   bits 1..   (price-table slot index + 1); 0 means "no active order"
    // The slot already holds the price, so the order doesn't store it, and
    // cancel can jump straight to its slot instead of hashing and probing.
    // Slots are claimed permanently and never move, so the index stays valid.

    struct PriceSlot {
        int32_t price;
        int32_t count;  // number of active orders resting at this price
    };

    static constexpr int64_t kMaxPrice = 1'000'000;
    static constexpr size_t kLevelsSize = kMaxPrice + 5;
    static constexpr size_t kChunks = kLevelsSize / 64 + 1;

    static constexpr size_t kOrdersFastSlots = 131'072;

    static constexpr size_t kPriceTableBits = 17;
    static constexpr size_t kPriceTableSlots = size_t(1) << kPriceTableBits;
    static constexpr size_t kPriceTableMask = kPriceTableSlots - 1;
    static constexpr uint64_t kHashMul = 0x9E3779B97F4A7C15ULL; // Fibonacci hashing

    static size_t hash_price(int64_t price) {
        return static_cast<size_t>((static_cast<uint64_t>(price) * kHashMul) >> (64 - kPriceTableBits));
    }

    // Returns the index in price_table_[side] of this price's slot, claiming
    // an empty one if the price isn't there yet.
    size_t find_or_create_slot(int side, int64_t price);

    std::array<uint64_t*, 2> bitmasks_;
    std::array<PriceSlot*, 2> price_table_;

    uint32_t* orders_fast_;

    int64_t best_[2];

};

} // namespace hftu
