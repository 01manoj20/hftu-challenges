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

    // price_table_/bitmasks_/orders_fast_ are raw new[] allocations, not
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
    struct Order {
        int32_t price;      // range [1, 1'000'000] fits comfortably
        uint16_t quantity;  // range [1, 10'000] fits comfortably
        uint8_t side;
        uint8_t active;     // explicit flag: quantity==0 is a valid input we must not alias
    };

    
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

    // Returns a pointer to this price's count,
    int32_t* find_or_create_slot(int side, int64_t price);
    
    // Returns a pointer to this price's count
    int32_t* find_existing_slot(int side, int64_t price);


  
    std::array<uint64_t*, 2> bitmasks_;
    std::array<PriceSlot*, 2> price_table_;

    Order* orders_fast_;

    int64_t best_[2];

};

} // namespace hftu
