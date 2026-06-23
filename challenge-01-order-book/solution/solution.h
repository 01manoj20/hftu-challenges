    #pragma once
    // Challenge 01: Order Book
    // Edit this file and solution.cpp to implement your solution.

    #include <cstdint>
    #include <vector>
    #include <algorithm>


    namespace hftu {

    class OrderBook {

    public:
        
        OrderBook();

        // Add a new order. side: 0=buy(bid), 1=sell(ask)
        void add_order(uint64_t id, int side, int64_t price, int64_t quantity);

        // Cancel an order by ID. No-op if ID doesn't exist.
        void cancel_order(uint64_t id);

        // Return highest bid price, or 0 if no bids.
        int64_t best_bid() const;

        // Return lowest ask price, or 0 if no asks.
        int64_t best_ask() const;

    private:
        // Replace these with your own data structures
        struct Order {
            int side;
            int64_t price;
            int64_t quantity;
        };

    // std::vector<int64_t> bids_qty_;
    // std::vector<int64_t> asks_qty_;
    std::vector<std::vector<int64_t>> levels_;
    std::vector<Order> orders_array_;
    std::vector<std::vector<uint64_t>> bitmasks_;

        mutable int64_t current_best_bid_;
        mutable int64_t current_best_ask_;


    };

    } // namespace hftu
