#pragma once
#include "Order.hpp"
#include "MemoryPool.hpp"
#include <iostream>
#include <map>
#include <unordered_map>
#include <functional>

// Branch Prediction Macros
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

enum class OrderType { LIMIT, MARKET, IOC, FOK };

class OrderBook {
public:
    using TradeCallback = std::function<void(uint64_t, uint64_t, uint64_t, uint32_t)>;

private:
    struct OrderList {
        Order* head = nullptr;
        Order* tail = nullptr;

        void push_back(Order* order) {
            order->next = nullptr;
            order->prev = tail;
            if (tail) tail->next = order;
            else head = order;
            tail = order;
        }

        void remove(Order* order) {
            if (order->prev) order->prev->next = order->next;
            else head = order->next;
            
            if (order->next) order->next->prev = order->prev;
            else tail = order->prev;
        }
        
        bool empty() const { return head == nullptr; }
    };

    MemoryPool<10240> pool_; // Increased pool capacity for safety
    std::map<uint64_t, OrderList> asks_;
    std::map<uint64_t, OrderList, std::greater<uint64_t>> bids_;
    std::unordered_map<uint64_t, Order*> order_map_;
    TradeCallback trade_callback_;

public:
    void set_trade_callback(TradeCallback cb) { trade_callback_ = std::move(cb); }

    void add_order(uint64_t id, uint64_t price, uint32_t qty, bool is_buy, OrderType type = OrderType::LIMIT) {
        if (unlikely(type == OrderType::FOK)) {
            if (!can_fill_completely(price, qty, is_buy)) {
                return; // Killed immediately
            }
        }

        Order* order = pool_.allocate();
        if (unlikely(!order)) return;

        order->order_id = id;
        order->price = price;
        order->quantity = qty;
        order->is_buy = is_buy;
        order->is_market = (type == OrderType::MARKET);

        order_map_[id] = order;

        if (likely(is_buy)) {
            match_buy(order, type);
        } else {
            match_sell(order, type);
        }
    }

    void print_book() const {
        std::cout << "--- ORDER BOOK ---\nAsks:\n";
        for (const auto& [price, list] : asks_) {
            Order* curr = list.head;
            while (curr) {
                std::cout << "  ID: " << curr->order_id << " | Price: " << price << " | Qty: " << curr->quantity << "\n";
                curr = curr->next;
            }
        }
        std::cout << "Bids:\n";
        for (const auto& [price, list] : bids_) {
            Order* curr = list.head;
            while (curr) {
                std::cout << "  ID: " << curr->order_id << " | Price: " << price << " | Qty: " << curr->quantity << "\n";
                curr = curr->next;
            }
        }
        std::cout << "------------------\n";
    }

private:
    bool can_fill_completely(uint64_t price, uint32_t qty, bool is_buy) const {
        uint32_t available = 0;
        if (is_buy) {
            for (const auto& [ask_price, list] : asks_) {
                if (price < ask_price) break;
                Order* curr = list.head;
                while (curr) {
                    available += curr->quantity;
                    if (available >= qty) return true;
                    curr = curr->next;
                }
            }
        } else {
            for (const auto& [bid_price, list] : bids_) {
                if (price > bid_price) break;
                Order* curr = list.head;
                while (curr) {
                    available += curr->quantity;
                    if (available >= qty) return true;
                    curr = curr->next;
                }
            }
        }
        return false;
    }

    void match_buy(Order* buy_order, OrderType type) {
        while (likely(buy_order->quantity > 0) && likely(!asks_.empty())) {
            auto best_ask_it = asks_.begin();
            uint64_t best_price = best_ask_it->first;
            OrderList& list = best_ask_it->second;
            Order* best_ask = list.head;

            if (likely(type == OrderType::MARKET || buy_order->is_market || buy_order->price >= best_price)) {
                uint32_t traded_qty = std::min(buy_order->quantity, best_ask->quantity);
                
                if (likely(trade_callback_ != nullptr)) {
                    trade_callback_(buy_order->order_id, best_ask->order_id, best_price, traded_qty);
                }

                buy_order->quantity -= traded_qty;
                best_ask->quantity -= traded_qty;

                if (unlikely(best_ask->quantity == 0)) {
                    list.remove(best_ask);
                    order_map_.erase(best_ask->order_id);
                    pool_.deallocate(best_ask);
                    if (list.empty()) asks_.erase(best_ask_it);
                }
            } else {
                break;
            }
        }

        if (unlikely(buy_order->quantity > 0)) {
            if (likely(type == OrderType::LIMIT)) {
                bids_[buy_order->price].push_back(buy_order);
                return; 
            }
        }
        
        if (buy_order->quantity == 0 || type != OrderType::LIMIT) {
            order_map_.erase(buy_order->order_id);
            pool_.deallocate(buy_order);
        }
    }

    void match_sell(Order* sell_order, OrderType type) {
        while (likely(sell_order->quantity > 0) && likely(!bids_.empty())) {
            auto best_bid_it = bids_.begin();
            uint64_t best_price = best_bid_it->first;
            OrderList& list = best_bid_it->second;
            Order* best_bid = list.head;

            if (likely(type == OrderType::MARKET || sell_order->is_market || sell_order->price <= best_price)) {
                uint32_t traded_qty = std::min(sell_order->quantity, best_bid->quantity);
                
                if (likely(trade_callback_ != nullptr)) {
                    trade_callback_(best_bid->order_id, sell_order->order_id, best_price, traded_qty);
                }

                sell_order->quantity -= traded_qty;
                best_bid->quantity -= traded_qty;

                if (unlikely(best_bid->quantity == 0)) {
                    list.remove(best_bid);
                    order_map_.erase(best_bid->order_id);
                    pool_.deallocate(best_bid);
                    if (list.empty()) bids_.erase(best_bid_it);
                }
            } else {
                break;
            }
        }

        if (unlikely(sell_order->quantity > 0)) {
            if (likely(type == OrderType::LIMIT)) {
                asks_[sell_order->price].push_back(sell_order);
                return;
            }
        }
        
        if (sell_order->quantity == 0 || type != OrderType::LIMIT) {
            order_map_.erase(sell_order->order_id);
            pool_.deallocate(sell_order);
        }
    }
};
