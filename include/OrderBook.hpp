#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>
#include <vector>

class OrderBook {
private:
    MemoryPool<1024> pool_;
    std::vector<Order*> bids_;
    std::vector<Order*> asks_;

public:
    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy) {
        Order* order = pool_.allocate();
        if (!order) {
            std::cout << "Memory pool full!\n";
            return;
        }
        order->order_id = id;
        order->price = price;
        order->quantity = qty;
        order->is_buy = is_buy;

        if (is_buy) {
            match_buy(order);
        } else {
            match_sell(order);
        }
    }

private:
    void match_buy(Order* buy_order) {
        std::cout << "Processing Buy Order: ID " << buy_order->order_id << " at " << buy_order->price << "\n";
        bids_.push_back(buy_order);
    }

    void match_sell(Order* sell_order) {
        std::cout << "Processing Sell Order: ID " << sell_order->order_id << " at " << sell_order->price << "\n";
        asks_.push_back(sell_order);
    }
};
