#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <unordered_map>

class OrderBook {
private:
    MemoryPool<1024> pool_;
    // Asks: lowest price first (ascending order)
    std::map<uint64_t, std::vector<Order*>> asks_;
    // Bids: highest price first (descending order)
    std::map<uint64_t, std::vector<Order*>, std::greater<uint64_t>> bids_;
    std::unordered_map<uint64_t, Order*> order_map_;

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

        if (order->is_buy) {
            auto price_it = bids_.find(order->price);
            if (price_it != bids_.end()) {
                auto& vec = price_it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), order), vec.end());
                if (vec.empty()) {
                    bids_.erase(price_it);
                }
            }
        } else {
            auto price_it = asks_.find(order->price);
            if (price_it != asks_.end()) {
                auto& vec = price_it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), order), vec.end());
                if (vec.empty()) {
                    asks_.erase(price_it);
                }
            }
        }

        pool_.deallocate(order);
        order_map_.erase(it);
        std::cout << "Cancelled order ID: " << id << "\n";
    }

    void print_book() const {
        std::cout << "--- ORDER BOOK ---\n";
        std::cout << "Asks:\n";
        for (const auto& [price, orders] : asks_) {
            for (const auto& ask : orders) {
                std::cout << "  ID: " << ask->order_id << " | Price: " << price << " | Qty: " << ask->quantity << "\n";
            }
        }
        std::cout << "Bids:\n";
        for (const auto& [price, orders] : bids_) {
            for (const auto& bid : orders) {
                std::cout << "  ID: " << bid->order_id << " | Price: " << price << " | Qty: " << bid->quantity << "\n";
            }
        }
        std::cout << "------------------\n";
    }

private:
    void match_buy(Order* buy_order) {
        while (buy_order->quantity > 0 && !asks_.empty()) {
            auto best_ask_it = asks_.begin();
            uint64_t best_price = best_ask_it->first;
            auto& ask_vector = best_ask_it->second;
            Order* best_ask = ask_vector.front();

            if (buy_order->is_market || buy_order->price >= best_price) {
                uint32_t traded_qty = std::min(buy_order->quantity, best_ask->quantity);
                std::cout << "TRADE: Executed " << traded_qty << " units at price " << best_price 
                          << " (Buy ID: " << buy_order->order_id << ", Sell ID: " << best_ask->order_id << ")\n";

                buy_order->quantity -= traded_qty;
                best_ask->quantity -= traded_qty;

                if (best_ask->quantity == 0) {
                    ask_vector.erase(ask_vector.begin());
                    order_map_.erase(best_ask->order_id);
                    pool_.deallocate(best_ask);

                    if (ask_vector.empty()) {
                        asks_.erase(best_ask_it);
                    }
                }
            } else {
                break;
            }
        }

        if (buy_order->quantity > 0 && !buy_order->is_market) {
            bids_[buy_order->price].push_back(buy_order);
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
            auto best_bid_it = bids_.begin();
            uint64_t best_price = best_bid_it->first;
            auto& bid_vector = best_bid_it->second;
            Order* best_bid = bid_vector.front();

            if (sell_order->is_market || sell_order->price <= best_price) {
                uint32_t traded_qty = std::min(sell_order->quantity, best_bid->quantity);
                std::cout << "TRADE: Executed " << traded_qty << " units at price " << best_price 
                          << " (Sell ID: " << sell_order->order_id << ", Buy ID: " << best_bid->order_id << ")\n";

                sell_order->quantity -= traded_qty;
                best_bid->quantity -= traded_qty;

                if (best_bid->quantity == 0) {
                    bid_vector.erase(bid_vector.begin());
                    order_map_.erase(best_bid->order_id);
                    pool_.deallocate(best_bid);

                    if (bid_vector.empty()) {
                        bids_.erase(best_bid_it);
                    }
                }
            } else {
                break;
            }
        }

        if (sell_order->quantity > 0 && !sell_order->is_market) {
            asks_[sell_order->price].push_back(sell_order);
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
