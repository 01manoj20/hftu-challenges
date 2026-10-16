// Challenge 01: Order Book

#include "solution.h"

namespace hftu {

OrderBook::OrderBook()
    : counts_{new int32_t[kLevelsSize](), new int32_t[kLevelsSize]()},
      bitmasks_{new uint64_t[kChunks](), new uint64_t[kChunks]()},
      orders_fast_(new uint32_t[kOrdersFastSlots]()),
      best_{0, INT64_MIN}
{}

OrderBook::~OrderBook() {
    delete[] counts_[0];
    delete[] counts_[1];
    delete[] bitmasks_[0];
    delete[] bitmasks_[1];
    delete[] orders_fast_;
}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t /*quantity*/) {
    uint32_t ref = (static_cast<uint32_t>(price) << 1) | static_cast<uint32_t>(side);
    if (id < kOrdersFastSlots) {
        orders_fast_[id] = ref;
    } else {
        orders_overflow_[id] = ref;
    }

    int32_t old_count = counts_[side][price]++;

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
    int32_t new_count = --counts_[side][price];

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
