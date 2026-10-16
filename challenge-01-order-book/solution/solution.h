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

    void add_order(uint64_t id, int side, int64_t price, int64_t quantity);
    void cancel_order(uint64_t id);
    int64_t best_bid() const { return best_[0]; }
    int64_t best_ask() const { return (best_[1] == INT64_MIN) ? 0 : -best_[1]; }

private:
    

    static constexpr int64_t kMaxPrice = 1'000'000;
    static constexpr size_t kLevelsSize = kMaxPrice + 5;
    static constexpr size_t kChunks = kLevelsSize / 64 + 1;


    

 
    // counts_[side][price] = number of active orders at that price, in 16 bits
    // (2 MB per side). It stops at 65,535; any orders beyond that are counted
    // in extra_ instead. The benchmark never has more than a handful of orders
    // at one price, so extra_ is never touched there, but the spec allows up
    // to 1,000,000 orders.
    //     true count = counts_[side][price] + extra_[side][price]
    // An extra_ entry only exists while counts_ is at 65,535, so
    // counts_ == 0 always means the price is empty.
    std::array<uint16_t*, 2> counts_;
    std::array<std::unordered_map<int64_t, uint32_t>, 2> extra_;

    std::array<uint64_t*, 2> bitmasks_;

    static constexpr size_t kOrdersFastSlots = 131'072;

    uint32_t* orders_fast_;
    std::unordered_map<uint64_t, uint32_t> orders_overflow_;  

    int64_t best_[2];

};

} // namespace hftu
