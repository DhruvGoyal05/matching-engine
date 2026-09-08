#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>

class OrderBook {
private:
    MemoryPool<1024> pool_;
    std::vector<Order*> bids_;
    std::vector<Order*> asks_;
    std::unordered_map<uint64_t, Order*> order_map_; // Fast O(1) lookup for cancellations

public:
    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, bool is_market = false) {
        Order* order = pool_.allocate();
        if (!order) {
            std::cout << "Memory pool full!\n";
            return;
        }
        order->order_id = id;
        order->price = price;
        order->quantity = qty;
        order->is_buy = is_buy;
        order->is_market = is_market;

        order_map_[id] = order;

        if (is_buy) {
            match_buy(order);
        } else {
            match_sell(order);
        }
    }

    void cancel_order(uint64_t id) {
        auto it = order_map_.find(id);
        if (it == order_map_.end()) {
            std::cout << "Cancel failed: Order ID " << id << " not found.\n";
            return;
        }

        Order* order = it->second;

        // Remove from bids_ or asks_ vector
        if (order->is_buy) {
            bids_.erase(std::remove(bids_.begin(), bids_.end(), order), bids_.end());
        } else {
            asks_.erase(std::remove(asks_.begin(), asks_.end(), order), asks_.end());
        }

        pool_.deallocate(order);
        order_map_.erase(it);
        std::cout << "Cancelled order ID: " << id << "\n";
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

            if (buy_order->is_market || buy_order->price >= best_ask->price) {
                uint32_t traded_qty = std::min(buy_order->quantity, best_ask->quantity);
                std::cout << "TRADE: Executed " << traded_qty << " units at price " << best_ask->price 
                          << " (Buy ID: " << buy_order->order_id << ", Sell ID: " << best_ask->order_id << ")\n";

                buy_order->quantity -= traded_qty;
                best_ask->quantity -= traded_qty;

                if (best_ask->quantity == 0) {
                    asks_.erase(asks_.begin());
                    order_map_.erase(best_ask->order_id);
                    pool_.deallocate(best_ask);
                }
            } else {
                break;
            }
        }

        if (buy_order->quantity > 0 && !buy_order->is_market) {
            bids_.push_back(buy_order);
            std::cout << "Limit Buy order added to book: ID " << buy_order->order_id << "\n";
        } else {
            if (buy_order->quantity > 0 && buy_order->is_market) {
                std::cout << "Market Buy order partially unfilled, cancelling remainder: ID " << buy_order->order_id << "\n";
            }
            order_map_.erase(buy_order->order_id);
            pool_.deallocate(buy_order);
        }
    }

    void match_sell(Order* sell_order) {
        while (sell_order->quantity > 0 && !bids_.empty()) {
            std::sort(bids_.begin(), bids_.end(), [](Order* a, Order* b) {
                return a->price > b->price;
            });

            Order* best_bid = bids_.front();

            if (sell_order->is_market || sell_order->price <= best_bid->price) {
                uint32_t traded_qty = std::min(sell_order->quantity, best_bid->quantity);
                std::cout << "TRADE: Executed " << traded_qty << " units at price " << best_bid->price 
                          << " (Sell ID: " << sell_order->order_id << ", Buy ID: " << best_bid->order_id << ")\n";

                sell_order->quantity -= traded_qty;
                best_bid->quantity -= traded_qty;

                if (best_bid->quantity == 0) {
                    bids_.erase(bids_.begin());
                    order_map_.erase(best_bid->order_id);
                    pool_.deallocate(best_bid);
                }
            } else {
                break;
            }
        }

        if (sell_order->quantity > 0 && !sell_order->is_market) {
            asks_.push_back(sell_order);
            std::cout << "Limit Sell order added to book: ID " << sell_order->order_id << "\n";
        } else {
            if (sell_order->quantity > 0 && sell_order->is_market) {
                std::cout << "Market Sell order partially unfilled, cancelling remainder: ID " << sell_order->order_id << "\n";
            }
            order_map_.erase(sell_order->order_id);
            pool_.deallocate(sell_order);
        }
    }
};
