// Challenge 01: Order Book — Skeleton Implementation
// This is a naive reference. You can do much better!

#include "solution.h"

namespace hftu {

void OrderBook::add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
    orders[id] = {side, price, quantity};
    if (side == 0) {
        bids[price] += quantity;
        best_bid = max(best_bid,price);
    } else {
        asks[price] += quantity;
        best_ask = min(best_ask,price);
    }
}

void OrderBook::cancel_order(uint64_t id) {
    // auto it = orders_.find(id);
    // if (it == orders_.end()) return;

    auto it = orders[id];
    if (side == 0) 
    {
        bids[it.price] -= it.quantity;
        if((it.price == best_bid))
        {
            if(bids[it.price] == 0)
            {
                for(int i=0;i<bids.size();i++)
                {
                    if(bids[i]!=0)
                        best_bid = i;
                }
            }
        }
    } 
    else 
    {
        asks[it.price] -= it.quantity;
        if((it.price) == best_ask)
        {
            if(asks[it.price] == 0)
            {
                for(int i = it.price-1;i>=0;i--)
                {
                    if(asks[i]!=0)
                    {
                        best_ask = i;
                        break;
                    }
                }
                if(best_ask == it.price)
                    best_ask = 0;
            }
        }
    }
    orders.erase(orders.begin()+id);


    // auto& order = it->second;
    // if (order.side == 0) {
    //     auto bit = bids_.find(order.price);
    //     if (bit != bids_.end()) {
    //         bit->second -= order.quantity;
    //         if (bit->second <= 0) bids_.erase(bit);
    //     }
    // } else {
    //     auto ait = asks_.find(order.price);
    //     if (ait != asks_.end()) {
    //         ait->second -= order.quantity;
    //         if (ait->second <= 0) asks_.erase(ait);
    //     }
    // }
    // orders_.erase(it);
}

int64_t OrderBook::best_bid() const {
    // return bids_.empty() ? 0 : bids_.begin()->first;
    return best_bid;
}

int64_t OrderBook::best_ask() const {
    // return asks_.empty() ? 0 : asks_.begin()->first;
    return best_ask;
}

} // namespace hftu
