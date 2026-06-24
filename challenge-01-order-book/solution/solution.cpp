// Challenge 01: Order Book — Skeleton Implementation
// This is a naive reference. You can do much better!

#include "solution.h"

namespace hftu {

    OrderBook::OrderBook(): 
    // : bids_qty_(1000005, 0), 
    //   asks_qty_(1000005, 0), 
      levels_(2, std::vector<int64_t>(1000005, 0)),
      bitmasks_(2, std::vector<uint64_t>(16000, 0)),
      orders_array_(100010), 
    // levels_{},
    // orders_array_{},
    // bitmasks_{},
    current_best_bid_(0), 
    current_best_ask_(INT64_MAX) // Initialize to max, not 0!
{}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
    orders_array_[id] = {side, price, quantity};
    // if (side == 0) { // Bid
    //     levels_[side][price] += quantity;
    //     current_best_bid_ = std::max(current_best_bid_, price);
    // } else { // Ask
    //     levels_[side][price] += quantity;
    //     if (quantity > 0) {
    //         current_best_ask_ = std::min(current_best_ask_, price);
    //     }
    // }
    if (levels_[side][price] == 0 && quantity > 0) {
        uint64_t chunk = price / 64;
        uint64_t bit = price % 64;
        bitmasks_[side][chunk] |= (1ULL << bit);
    }

    levels_[side][price] += quantity;

    if(side == 0) // Bid
        current_best_bid_ = std::max(current_best_bid_, price);
    else 
        current_best_ask_ = std::min(current_best_ask_, price);
        
    
}

void OrderBook::cancel_order(uint64_t id) {
    

    auto& ord = orders_array_[id];
    if (ord.quantity == 0) return;

    levels_[ord.side][ord.price] -= ord.quantity;

    if (levels_[ord.side][ord.price] == 0) 
    {
        
        uint64_t chunk = ord.price / 64;
        uint64_t bit = ord.price % 64;
        
        bitmasks_[ord.side][chunk] &= ~(1ULL << bit);

        if (ord.side == 0) 
        {
            if (ord.price == current_best_bid_) 
            {
                current_best_bid_ = 0; 
                uint64_t mask = bitmasks_[0][chunk] & ((1ULL << bit) - 1);
                
                if (mask != 0) {
                    current_best_bid_ = (chunk * 64) + (63 - __builtin_clzll(mask));
                } 
                else {
                    for (int64_t c = chunk - 1; c >= 0; c--) {
                        if (bitmasks_[0][c] != 0) {
                            current_best_bid_ = (c * 64) + (63 - __builtin_clzll(bitmasks_[0][c]));
                            break;
                        }
                    }
                }
            }
        }
        else 
        {
            if (ord.price == current_best_ask_) 
            {
                current_best_ask_ = INT64_MAX; 
                uint64_t mask = (bit == 63) ? 0 : (bitmasks_[1][chunk] & (~0ULL << (bit + 1)));
                
                if (mask != 0) {
                    current_best_ask_ = (chunk * 64) + __builtin_ctzll(mask);
                } 
                else {
                    for (size_t c = chunk + 1; c < 16000; c++) {
                        if (bitmasks_[1][c] != 0) {
                            current_best_ask_ = (c * 64) + __builtin_ctzll(bitmasks_[1][c]);
                            break;
                        }
                    }
                }
            }
        }
    }

    ord.quantity = 0; // Mark as canceled

}

int64_t OrderBook::best_bid() const {
    // return bids_.empty() ? 0 : bids_.begin()->first;
    return current_best_bid_;
}

int64_t OrderBook::best_ask() const {
    // return asks_.empty() ? 0 : asks_.begin()->first;
    return current_best_ask_ == INT64_MAX ? 0 : current_best_ask_;
}

} // namespace hftu