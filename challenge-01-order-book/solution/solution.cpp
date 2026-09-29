// Challenge 01: Order Book

#include "solution.h"

namespace hftu {

OrderBook::OrderBook()
    : levels_{std::vector<int64_t>(kLevelsSize, 0), std::vector<int64_t>(kLevelsSize, 0)},
      bitmasks_{std::vector<uint64_t>(kChunks, 0), std::vector<uint64_t>(kChunks, 0)},
      orders_array_(kMaxOrders),
      best_{0,INT64_MIN}
    //   current_best_ask_(INT64_MAX) 
{}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
    orders_array_[id] = {static_cast<int32_t>(price), static_cast<uint16_t>(quantity),
                          static_cast<uint8_t>(side), 1};

    int64_t& lvl = levels_[side][price];
    if (lvl == 0) {
        uint64_t chunk = price / 64;
        uint64_t bit = price % 64;
        bitmasks_[side][chunk] |= (1ULL << bit);
    }
    lvl += quantity;

    int64_t signed_price = (side == 0) ? price : -price;
    best_[side] = std::max(best_[side],signed_price);


    // if (side == 0) 
    //     current_best_bid_ = std::max(current_best_bid_, price);
    // else
    //     current_best_ask_ = std::min(current_best_ask_, price);
}

void OrderBook::cancel_order(uint64_t id) {

    auto& ord = orders_array_[id];
    if (!ord.active) return;

    int64_t price = ord.price;
    int side = ord.side;

    int64_t& lvl = levels_[side][price];
    lvl -= ord.quantity;

    if (lvl == 0)
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

    ord.active = 0; // Mark as canceled

}

int64_t OrderBook::best_bid() const {
    return best_[0];
}

int64_t OrderBook::best_ask() const {
    return (best_[1] == INT64_MIN) ? 0:-best_[1];
}

} // namespace hftu
