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
    static constexpr size_t kWords = (kLevelsSize + 31) / 32;     // 32 prices per 64-bit word

    // counts_[side] holds the number of active orders at every price as a
    // 2-bit field, 32 fields per 64-bit word (250 KB per side):
    //     price p lives in word p >> 5, bits [2*(p & 31), 2*(p & 31) + 1]
    // A field counts 0, 1, 2 or 3. Orders beyond 3 at one price are counted in
    // extra_, so
    //     true count = field + extra_[side][price]
    // An extra_ entry only exists while the field is 3, so a field of 0 always
    // means the price is empty. That also makes a separate "which prices are
    // occupied" bitmap unnecessary: a price is occupied exactly when its field
    // is non-zero, and finding the next best price scans these same words.
    std::array<uint64_t*, 2> counts_;
    std::array<std::unordered_map<int64_t, uint32_t>, 2> extra_;

    static constexpr size_t kOrdersFastSlots = 131'072;

    uint32_t* orders_fast_;
    std::unordered_map<uint64_t, uint32_t> orders_overflow_;  

    int64_t best_[2];

};

} // namespace hftu
