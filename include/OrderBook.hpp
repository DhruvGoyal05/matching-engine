#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <functional>

class OrderBook {
public:
    // Callback signature for trade executions (Buy ID, Sell ID, Price, Quantity)
    using TradeCallback = std::function<void(uint64_t, uint64_t, uint64_t, uint32_t)>;

private:
    MemoryPool<1024> pool_;
    std::map<uint64_t, std::vector<Order*>> asks_;
    std::map<uint64_t, std::vector<Order*>, std::greater<uint64_t>> bids_;
    std::unordered_map<uint64_t, Order*> order_map_;
    TradeCallback trade_callback_;

public:
    // Register a zero-I/O callback sink for trades
    void set_trade_callback(TradeCallback cb) {
        trade_callback_ = std::move(cb);
    }

    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, bool is_market = false) {
        Order* order = pool_.allocate();
        if (!order) return; // Silent failure in production path
        
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
        if (it == order_map_.end()) return;

        Order* order = it->second;

        if (order->is_buy) {
            auto price_it = bids_.find(order->price);
            if (price_it != bids_.end()) {
                auto& vec = price_it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), order), vec.end());
                if (vec.empty()) bids_.erase(price_it);
            }
        } else {
            auto price_it = asks_.find(order->price);
            if (price_it != asks_.end()) {
                auto& vec = price_it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), order), vec.end());
                if (vec.empty()) asks_.erase(price_it);
            }
        }

        pool_.deallocate(order);
        order_map_.erase(it);
    }

    void print_book() const {
        std::cout << "--- ORDER BOOK ---\nAsks:\n";
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
                
                // Invoke callback instead of direct std::cout
                if (trade_callback_) {
                    trade_callback_(buy_order->order_id, best_ask->order_id, best_price, traded_qty);
                }

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
        } else {
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
                
                // Invoke callback instead of direct std::cout
                if (trade_callback_) {
                    trade_callback_(best_bid->order_id, sell_order->order_id, best_price, traded_qty);
                }

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
        } else {
            order_map_.erase(sell_order->order_id);
            pool_.deallocate(sell_order);
        }
    }
};
