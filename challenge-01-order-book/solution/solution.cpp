// Challenge 01: Order Book — Skeleton Implementation
// This is a naive reference. You can do much better!

#include "solution.h"

namespace hftu {

    OrderBook::OrderBook(): 
        levels_{},        
        bitmasks_{},      
        orders_array_{},
        current_best_bid_(0), 
        current_best_ask_(INT64_MAX) // Initialize to max, not 0!
{}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
    orders_array_[id] = {side, price, quantity};
    bitmasks_[side][price >> 6] |= (1ULL << (price & 63));

    levels_[side][price] += quantity;

    if(side == 0) // Bid
        if (price > current_best_bid_) current_best_bid_ = price;
    else 
        if (price < current_best_ask_) current_best_ask_ = price;
        
    
}

void OrderBook::cancel_order(uint64_t id) {
    

    auto& ord = orders_array_[id];
    if (ord.quantity == 0) return;

    levels_[ord.side][ord.price] -= ord.quantity;

    if (levels_[ord.side][ord.price] == 0) 
        bitmasks_[ord.side][ord.price >> 6] &= ~(1ULL << (ord.price & 63));

    ord.quantity = 0; // Mark as canceled

}

int64_t OrderBook::best_bid() const {
  
    if (levels_[0][current_best_bid_] > 0) {
        return current_best_bid_;
    }

    int64_t chunk = current_best_bid_ >> 6;
    uint64_t bit = current_best_bid_ & 63;
    
    uint64_t mask = bitmasks_[0][chunk] & ((1ULL << bit) - 1);
    if (mask != 0) {
        current_best_bid_ = (chunk << 6) + (63 - __builtin_clzll(mask));
        return current_best_bid_;
    }

    for (int64_t c = chunk - 1; c >= 0; c--) {
        if (bitmasks_[0][c] != 0) {
            current_best_bid_ = (c << 6) + (63 - __builtin_clzll(bitmasks_[0][c]));
            return current_best_bid_;
        }
    }

    // Book is completely empty
    current_best_bid_ = 0;
    return 0;
}

int64_t OrderBook::best_ask() const {
   
    if (current_best_ask_ == INT64_MAX) return 0; 
    
    if (levels_[1][current_best_ask_] > 0) {
        return current_best_ask_;
    }

    int64_t chunk = current_best_ask_ >> 6;
    uint64_t bit = current_best_ask_ & 63;
    
   
    uint64_t mask = (bit == 63) ? 0 : (bitmasks_[1][chunk] & (~0ULL << (bit + 1)));
    if (mask != 0) {
        current_best_ask_ = (chunk << 6) + __builtin_ctzll(mask);
        return current_best_ask_;
    }

    for (int64_t c = chunk + 1; c < 16000; c++) {
        if (bitmasks_[1][c] != 0) {
            current_best_ask_ = (c << 6) + __builtin_ctzll(bitmasks_[1][c]);
            return current_best_ask_;
        }
    }

    // Book is completely empty
    current_best_ask_ = INT64_MAX;
    return 0;
}

} // namespace hftu
