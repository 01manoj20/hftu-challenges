// Challenge 01: Order Book

#include "solution.h"

namespace hftu {

void OrderBook::init_table(PriceTable& t, unsigned bits) {
    t.buckets = new Bucket[size_t(1) << bits]();
    t.mask = (size_t(1) << bits) - 1;
    t.shift = 64 - bits;
    t.claimed = 0;
    t.grow_at = kMaxAvgPerBucket << bits;
}

OrderBook::OrderBook()
    : bitmasks_{new uint64_t[kChunks](), new uint64_t[kChunks]()},
      orders_fast_(new uint32_t[kOrdersFastSlots]()),
      best_{0, INT64_MIN}
{
    init_table(counts_[0], kInitialBucketBits);
    init_table(counts_[1], kInitialBucketBits);
}

OrderBook::~OrderBook() {
    delete[] counts_[0].buckets;
    delete[] counts_[1].buckets;
    delete[] bitmasks_[0];
    delete[] bitmasks_[1];
    delete[] orders_fast_;
}

// Bit i of `match` is set if slot i holds `price`; bit i of `empty` if slot i is free.
static inline void scan_bucket(const int32_t* prices, int32_t price, unsigned& match, unsigned& empty) {
    match = 0;
    empty = 0;
    for (int i = 0; i < 8; ++i) {
        match |= static_cast<unsigned>(prices[i] == price) << i;
        empty |= static_cast<unsigned>(prices[i] == 0) << i;
    }
}

int32_t* OrderBook::find_or_claim(int side, int64_t price) {
    PriceTable& t = counts_[side];
    const int32_t p = static_cast<int32_t>(price);
    size_t b = static_cast<size_t>((static_cast<uint64_t>(price) * kHashMul) >> t.shift);
    for (;;) {
        Bucket& bk = t.buckets[b];
        unsigned match, empty;
        scan_bucket(bk.price, p, match, empty);
        if (match) return &bk.count[__builtin_ctz(match)];
        if (empty) {
            if (t.claimed >= t.grow_at) {       // rare: table is getting full
                grow(side);
                return find_or_claim(side, price);
            }
            int i = __builtin_ctz(empty);
            bk.price[i] = p;
            bk.count[i] = 0;
            ++t.claimed;
            return &bk.count[i];
        }
        b = (b + 1) & t.mask;                   // bucket full: try the next one
    }
}

int32_t* OrderBook::find_existing(int side, int64_t price) {
    const PriceTable& t = counts_[side];
    const int32_t p = static_cast<int32_t>(price);
    size_t b = static_cast<size_t>((static_cast<uint64_t>(price) * kHashMul) >> t.shift);
    for (;;) {
        Bucket& bk = t.buckets[b];
        unsigned match, empty;
        scan_bucket(bk.price, p, match, empty);
        if (match) return &bk.count[__builtin_ctz(match)];
        if (empty) return nullptr;
        b = (b + 1) & t.mask;
    }
}

void OrderBook::grow(int side) {
    PriceTable old = counts_[side];
    PriceTable bigger;
    init_table(bigger, (64 - old.shift) + 1);
    for (size_t b = 0; b <= old.mask; ++b) {
        const Bucket& ob = old.buckets[b];
        for (int i = 0; i < kSlotsPerBucket; ++i) {
            if (ob.price[i] == 0 || ob.count[i] == 0) continue;   // empty, or a price with no orders left
            size_t nb = static_cast<size_t>((static_cast<uint64_t>(ob.price[i]) * kHashMul) >> bigger.shift);
            for (;;) {
                Bucket& bk = bigger.buckets[nb];
                unsigned match, empty;
                scan_bucket(bk.price, 0, match, empty);   // looking for a free slot (price 0)
                if (match) {
                    int j = __builtin_ctz(match);
                    bk.price[j] = ob.price[i];
                    bk.count[j] = ob.count[i];
                    ++bigger.claimed;
                    break;
                }
                nb = (nb + 1) & bigger.mask;
            }
        }
    }
    delete[] old.buckets;
    counts_[side] = bigger;
}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t /*quantity*/) {
    uint32_t ref = (static_cast<uint32_t>(price) << 1) | static_cast<uint32_t>(side);
    if (id < kOrdersFastSlots) {
        orders_fast_[id] = ref;
    } else {
        orders_overflow_[id] = ref;
    }

    int32_t old_count = (*find_or_claim(side, price))++;

    if (old_count == 0) {
        uint64_t chunk = price / 64;
        uint64_t bit = price % 64;
        bitmasks_[side][chunk] |= (1ULL << bit);
    }

    int64_t signed_price = (side == 0) ? price : -price;
    best_[side] = std::max(best_[side], signed_price);
}

void OrderBook::cancel_order(uint64_t id) {

    uint32_t ref;
    if (id < kOrdersFastSlots) {
        ref = orders_fast_[id];
    } else {
        auto it = orders_overflow_.find(id);
        if (it == orders_overflow_.end()) return;   // unknown id: no-op
        ref = it->second;
    }
    if (ref == 0) return;                           // unknown or already cancelled: no-op

    int side = ref & 1;
    int64_t price = ref >> 1;
    int32_t* count = find_existing(side, price);
    if (!count) __builtin_trap();                   // a live order's price is always in the table
    int32_t new_count = --*count;

    if (new_count == 0)
    {
        uint64_t chunk = price / 64;
        uint64_t bit = price % 64;

        bitmasks_[side][chunk] &= ~(1ULL << bit);

        if (side == 0)
        {
            if (price == best_[0])
            {
                best_[0] = 0;
                uint64_t mask = bitmasks_[0][chunk] & ((1ULL << bit) - 1);

                if (mask != 0) {
                    best_[0] = (chunk * 64) + (63 - __builtin_clzll(mask));
                }
                else {
                    for (int64_t c = chunk - 1; c >= 0; c--) {
                        if (bitmasks_[0][c] != 0) {
                            best_[0] = (c * 64) + (63 - __builtin_clzll(bitmasks_[0][c]));
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            if (price == -best_[1])
            {
                best_[1] = INT64_MIN;
                uint64_t mask = (bit == 63) ? 0 : (bitmasks_[1][chunk] & (~0ULL << (bit + 1)));

                if (mask != 0) {
                    best_[1] = -((chunk * 64) + __builtin_ctzll(mask));
                }
                else {
                    for (size_t c = chunk + 1; c < kChunks; c++) {
                        if (bitmasks_[1][c] != 0) {
                            best_[1] = -((c * 64) + __builtin_ctzll(bitmasks_[1][c]));
                            break;
                        }
                    }
                }
            }
        }
    }

    // Mark as canceled
    if (id < kOrdersFastSlots) {
        orders_fast_[id] = 0;
    } else {
        orders_overflow_.erase(id);
    }

}

} // namespace hftu
