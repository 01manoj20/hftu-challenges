// Challenge 01: Order Book — Skeleton Implementation
// This is a naive reference. You can do much better!

#include "solution.h"

namespace hftu {

    OrderBook::OrderBook(): 
    // : bids_qty_(1000005, 0), 
    //   asks_qty_(1000005, 0), 
      levels_(2, std::vector<int64_t>(1000005, 0)),
      orders_array_(100010), 
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
    
    if (ord.side == 0) 
    {
        if (ord.price == current_best_bid_ && levels_[ord.side][ord.price] == 0) 
        {
            current_best_bid_ = 0; 
            for (int64_t i = ord.price - 1; i >= 0; i--) {
                if (levels_[ord.side][i] > 0) {
                    current_best_bid_ = i;
                    break;
                }
            }
        }
    }
    else 
    {
        
        if (ord.price == current_best_ask_ && levels_[ord.side][ord.price] == 0) 
        {
            current_best_ask_ = INT64_MAX; 
            for (size_t i = ord.price + 1; i < levels_[ord.side].size(); i++) {
                if (levels_[ord.side][i] > 0) {
                    current_best_ask_ = i;
                    break;
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
