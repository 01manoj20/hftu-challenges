// Challenge 01: Order Book

#include "solution.h"

namespace hftu {

// Bit 2j is set iff the 2-bit field j of w is non-zero (all other bits are 0).
static inline uint64_t occupied(uint64_t w) {
    return (w | (w >> 1)) & 0x5555555555555555ULL;
}

OrderBook::OrderBook()
    : counts_{new uint64_t[kWords](), new uint64_t[kWords]()},
      orders_fast_(new uint32_t[kOrdersFastSlots]()),
      best_{0, INT64_MIN}
{}

OrderBook::~OrderBook() {
    delete[] counts_[0];
    delete[] counts_[1];
    delete[] orders_fast_;
}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t /*quantity*/) {
    uint32_t ref = (static_cast<uint32_t>(price) << 1) | static_cast<uint32_t>(side);
    if (id < kOrdersFastSlots) {
        orders_fast_[id] = ref;
    } else {
        orders_overflow_[id] = ref;
    }

    uint64_t& word = counts_[side][price >> 5];
    const unsigned shift = (price & 31) << 1;
    const unsigned c = (word >> shift) & 3;
    if (__builtin_expect(c == 3, 0)) {              // field is full: the extra orders go in extra_
        ++extra_[side][price];
    } else {
        word += 1ULL << shift;                      // c < 3, so no carry into the next field
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
    uint64_t& word = counts_[side][price >> 5];
    const unsigned shift = (price & 31) << 1;
    const unsigned c = (word >> shift) & 3;
    bool now_empty = false;
    if (__builtin_expect(c == 3, 0)) {              // field is full: take from extra_ first
        auto it = extra_[side].find(price);
        if (it == extra_[side].end()) {
            word -= 1ULL << shift;                  // exactly 3 orders: now 2
        } else if (--it->second == 0) {
            extra_[side].erase(it);                 // field stays at 3
        }
    } else {
        word -= 1ULL << shift;                      // c >= 1 for a live order, so no borrow
        now_empty = (c == 1);
    }

    if (now_empty)
    {
        const uint64_t* words = counts_[side];
        const size_t wi = price >> 5;
        const unsigned j = price & 31;

        if (side == 0)
        {
            if (price == best_[0])
            {
                best_[0] = 0;
                uint64_t nz = occupied(words[wi]) & ((1ULL << (2 * j)) - 1);   // fields below this price
                if (nz != 0) {
                    best_[0] = (wi << 5) + ((63 - __builtin_clzll(nz)) >> 1);
                }
                else {
                    for (int64_t w = static_cast<int64_t>(wi) - 1; w >= 0; w--) {
                        uint64_t m = occupied(words[w]);
                        if (m != 0) {
                            best_[0] = (w << 5) + ((63 - __builtin_clzll(m)) >> 1);
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
                uint64_t nz = occupied(words[wi]) & ~((2ULL << (2 * j)) - 1);  // fields above this price
                if (nz != 0) {
                    best_[1] = -static_cast<int64_t>((wi << 5) + (__builtin_ctzll(nz) >> 1));
                }
                else {
                    for (size_t w = wi + 1; w < kWords; w++) {
                        uint64_t m = occupied(words[w]);
                        if (m != 0) {
                            best_[1] = -static_cast<int64_t>((w << 5) + (__builtin_ctzll(m) >> 1));
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
