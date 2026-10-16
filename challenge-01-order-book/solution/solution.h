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

    // counts_[side] maps price -> number of active orders resting at that
    // price. It is a hash table whose buckets are exactly one 64-byte cache
    // line: 8 prices followed by their 8 counts (price 0 = empty slot).
    // A lookup reads one line and compares all 8 prices inside it.
    //
    // A price goes into the first bucket (starting from its hash, moving to
    // the next bucket when one is full) that has an empty slot. Slots are
    // never freed, so a bucket that still has an empty slot has never
    // overflowed: finding an empty slot without a match means "not present".
    //
    // The table doubles when it gets too full (dropping prices whose count
    // is 0), so it works for any number of prices; kInitialBucketBits only
    // sets where it starts.
    static constexpr int kSlotsPerBucket = 8;
    struct alignas(64) Bucket {
        int32_t price[kSlotsPerBucket];
        int32_t count[kSlotsPerBucket];
    };
    static_assert(sizeof(Bucket) == 64, "a bucket must be exactly one cache line");

    struct PriceTable {
        Bucket* buckets;
        size_t mask;      // number of buckets - 1
        unsigned shift;   // 64 - log2(number of buckets)
        size_t claimed;   // slots in use, including ones whose count is 0
        size_t grow_at;   // double the table when claimed reaches this
    };

    static constexpr unsigned kInitialBucketBits = 13;   // 8192 buckets = 512 KB per side
    static constexpr size_t kMaxAvgPerBucket = 5;        // of 8 slots
    static constexpr uint64_t kHashMul = 0x9E3779B97F4A7C15ULL;

    static void init_table(PriceTable& t, unsigned bits);
    int32_t* find_or_claim(int side, int64_t price);   // may grow the table
    int32_t* find_existing(int side, int64_t price);   // nullptr if absent
    void grow(int side);

    std::array<PriceTable, 2> counts_;
    std::array<uint64_t*, 2> bitmasks_;

    static constexpr size_t kOrdersFastSlots = 131'072;

    uint32_t* orders_fast_;
    std::unordered_map<uint64_t, uint32_t> orders_overflow_;  

    int64_t best_[2];

};

} // namespace hftu
