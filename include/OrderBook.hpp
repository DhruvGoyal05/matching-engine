#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>

class OrderBook {
private:
    MemoryPool<1024> pool_;

public:
    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy) {
        Order* order = pool_.allocate();
        if (!order) {
            std::cout << "Memory pool full! Cannot add order.\n";
            return;
        }
        order->order_id = id;
        order->price = price;
        order->quantity = qty;
        order->is_buy = is_buy;

        std::cout << (is_buy ? "Buy" : "Sell") << " order added: ID " 
                  << id << " at price " << price << "\n";
    }
};
