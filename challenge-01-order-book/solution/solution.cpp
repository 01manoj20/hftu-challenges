// Challenge 01: Order Book — Skeleton Implementation
// This is a naive reference. You can do much better!

#include "solution.h"

namespace hftu {

    OrderBook::OrderBook() 
    : bids_qty_(1000005, 0), 
      asks_qty_(1000005, 0), 
      orders_array_(100010), 
      current_best_bid_(0), 
      current_best_ask_(INT64_MAX) // Initialize to max, not 0!
{}

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
    orders_array_[id] = {side, price, quantity};
    if (side == 0) { // Bid
        bids_qty_[price] += quantity;
        current_best_bid_ = std::max(current_best_bid_, price);
    } else { // Ask
        asks_qty_[price] += quantity;
        current_best_ask_ = std::min(current_best_ask_, price);
    }
}

void OrderBook::cancel_order(uint64_t id) {
    

    auto& ord = orders_array_[id];
    if (ord.quantity == 0) return;

    if (ord.side == 0) 
    {
        bids_qty_[ord.price] -= ord.quantity;
        if (ord.price == current_best_bid_ && bids_qty_[ord.price] == 0) 
        {
            current_best_bid_ = 0; 
            for (int64_t i = ord.price - 1; i >= 0; i--) {
                if (bids_qty_[i] > 0) {
                    current_best_bid_ = i;
                    break;
                }
            }
        }
    }
    else 
    {
        asks_qty_[ord.price] -= ord.quantity;
        if (ord.price == current_best_ask_ && asks_qty_[ord.price] == 0) 
        {
            current_best_ask_ = INT64_MAX; 
            for (size_t i = ord.price + 1; i < asks_qty_.size(); i++) {
                if (asks_qty_[i] > 0) {
                    current_best_ask_ = i;
                    break;
                }
            }
        }
    }

    orders_array_.erase(orders_array_.begin() + id);

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
