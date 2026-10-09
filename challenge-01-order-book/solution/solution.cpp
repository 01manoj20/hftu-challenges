// Challenge 01: Order Book

#include "solution.h"

namespace hftu {

OrderBook::OrderBook()
    : bitmasks_{new uint64_t[kChunks](), new uint64_t[kChunks]()},
      price_table_{new PriceSlot[kPriceTableSlots](), new PriceSlot[kPriceTableSlots]()},
      orders_fast_(new Order[kOrdersFastSlots]()),
      best_{0, INT64_MIN}
{}

OrderBook::~OrderBook() {
    delete[] bitmasks_[0];
    delete[] bitmasks_[1];
    delete[] price_table_[0];
    delete[] price_table_[1];
    delete[] orders_fast_;
}

int32_t* OrderBook::find_or_create_slot(int side, int64_t price) {
    PriceSlot* table = price_table_[side];
    size_t idx = hash_price(price);
    for (size_t probes = 0; probes < kPriceTableSlots; ++probes) {
        PriceSlot& slot = table[idx];
        if (slot.price == price) return &slot.count;
        if (slot.price == 0) {
            slot.price = static_cast<int32_t>(price);
            slot.count = 0;
            return &slot.count;
        }
        idx = (idx + 1) & kPriceTableMask;
    }
    return nullptr; // every slot permanently claimed by some other price
}

int32_t* OrderBook::find_existing_slot(int side, int64_t price) {
    PriceSlot* table = price_table_[side];
    size_t idx = hash_price(price);
    for (size_t probes = 0; probes < kPriceTableSlots; ++probes) {
        PriceSlot& slot = table[idx];
        if (slot.price == price) return &slot.count;
        if (slot.price == 0) return nullptr; // never claimed here -> must be in overflow_
        idx = (idx + 1) & kPriceTableMask;
    }
    return nullptr;
}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
    Order new_order = {static_cast<int32_t>(price), static_cast<uint16_t>(quantity),
                        static_cast<uint8_t>(side), 1};
    if (id < kOrdersFastSlots) {
        orders_fast_[id] = new_order;
    } else {
        orders_overflow_[id] = new_order;
    }

    int32_t old_count;
    if (int32_t* slot = find_or_create_slot(side, price)) {
        old_count = (*slot)++;
    } else {
        int32_t& cnt = overflow_[side][price]; // 0 if this is a new key
        old_count = cnt++;
    }

    if (old_count == 0) {
        uint64_t chunk = price / 64;
        uint64_t bit = price % 64;
        bitmasks_[side][chunk] |= (1ULL << bit);
    }

    int64_t signed_price = (side == 0) ? price : -price;
    best_[side] = std::max(best_[side], signed_price);
}

void OrderBook::cancel_order(uint64_t id) {

    Order* ord;
    if (id < kOrdersFastSlots) {
        ord = &orders_fast_[id];
    } else {
        auto it = orders_overflow_.find(id);
        if (it == orders_overflow_.end()) return; // never added -> no-op, per spec
        ord = &it->second;
    }
    if (!ord->active) return;

    int64_t price = ord->price;
    int side = ord->side;

    int32_t new_count;
    if (int32_t* slot = find_existing_slot(side, price)) {
        new_count = --(*slot);
    } else {
        // Must exist: this price was necessarily added through add_order's
        // identical find_or_create_slot/overflow_ logic before.
        auto it = overflow_[side].find(price);
        new_count = --(it->second);
    }

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

    ord->active = 0; // Mark as canceled

}

int64_t OrderBook::best_bid() const {
    return best_[0];
}

int64_t OrderBook::best_ask() const {
    return (best_[1] == INT64_MIN) ? 0 : -best_[1];
}

} // namespace hftu
