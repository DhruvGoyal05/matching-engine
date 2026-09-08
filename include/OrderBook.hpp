#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

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

    void print_book() const {
        std::cout << "--- ORDER BOOK ---\n";
        std::cout << "Asks:\n";
        for (const auto& ask : asks_) {
            std::cout << "  ID: " << ask->order_id << " | Price: " << ask->price << " | Qty: " << ask->quantity << "\n";
        }
        std::cout << "Bids:\n";
        for (const auto& bid : bids_) {
            std::cout << "  ID: " << bid->order_id << " | Price: " << bid->price << " | Qty: " << bid->quantity << "\n";
        }
        std::cout << "------------------\n";
    }

private:
    void match_buy(Order* buy_order) {
        while (buy_order->quantity > 0 && !asks_.empty()) {
            std::sort(asks_.begin(), asks_.end(), [](Order* a, Order* b) {
                return a->price < b->price;
            });

            Order* best_ask = asks_.front();

            if (buy_order->price >= best_ask->price) {
                uint32_t traded_qty = std::min(buy_order->quantity, best_ask->quantity);
                std::cout << "TRADE: Executed " << traded_qty << " units at price " << best_ask->price 
                          << " (Buy ID: " << buy_order->order_id << ", Sell ID: " << best_ask->order_id << ")\n";

                buy_order->quantity -= traded_qty;
                best_ask->quantity -= traded_qty;

                if (best_ask->quantity == 0) {
                    asks_.erase(asks_.begin());
                    pool_.deallocate(best_ask);
                }
            } else {
                break;
            }
        }

        if (buy_order->quantity > 0) {
            bids_.push_back(buy_order);
            std::cout << "Buy order added to book: ID " << buy_order->order_id << "\n";
        } else {
            pool_.deallocate(buy_order);
        }
    }

    void match_sell(Order* sell_order) {
        while (sell_order->quantity > 0 && !bids_.empty()) {
            std::sort(bids_.begin(), bids_.end(), [](Order* a, Order* b) {
                return a->price > b->price;
            });

            Order* best_bid = bids_.front();

            if (sell_order->price <= best_bid->price) {
                uint32_t traded_qty = std::min(sell_order->quantity, best_bid->quantity);
                std::cout << "TRADE: Executed " << traded_qty << " units at price " << best_bid->price 
                          << " (Sell ID: " << sell_order->order_id << ", Buy ID: " << best_bid->order_id << ")\n";

                sell_order->quantity -= traded_qty;
                best_bid->quantity -= traded_qty;

                if (best_bid->quantity == 0) {
                    bids_.erase(bids_.begin());
                    pool_.deallocate(best_bid);
                }
            } else {
                break;
            }
        }

        if (sell_order->quantity > 0) {
            asks_.push_back(sell_order);
            std::cout << "Sell order added to book: ID " << sell_order->order_id << "\n";
        } else {
            pool_.deallocate(sell_order);
        }
    }
};
