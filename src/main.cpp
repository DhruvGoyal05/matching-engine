#include "OrderBook.hpp"
#include "SPSCQueue.hpp"
#include <iostream>
#include <thread>
#include <chrono>

struct OrderCommand {
    uint64_t id;
    uint64_t price;
    uint32_t qty;
    bool is_buy;
    bool is_market;
    bool is_cancel;
};

int main() {
    OrderBook book;
    book.set_trade_callback([](uint64_t buy_id, uint64_t sell_id, uint64_t price, uint32_t qty) {
        std::cout << "[TRADE] Executed " << qty << " units at " << price 
                  << " (Buy ID: " << buy_id << ", Sell ID: " << sell_id << ")\n";
    });

    // SPSC Queue with capacity of 1024 (must be power of 2)
    SPSCQueue<OrderCommand, 1024> queue;
    std::atomic<bool> producer_done{false};

    // Consumer Thread: Matching Engine Core
    std::thread consumer([&]() {
        OrderCommand cmd;
        while (!producer_done.load(std::memory_order_relaxed) || queue.pop(cmd)) {
            if (queue.pop(cmd)) {
                if (cmd.is_cancel) {
                    book.cancel_order(cmd.id);
                } else {
                    book.add_order(cmd.id, cmd.price, cmd.qty, cmd.is_buy, cmd.is_market);
                }
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Producer Thread: Simulating Network Ingress
    std::thread producer([&]() {
        queue.push({1, 10050, 10, true, false, false}); // Limit Buy
        queue.push({2, 10050, 4, false, false, false});  // Limit Sell (triggers trade)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        producer_done.store(true, std::memory_order_relaxed);
    });

    producer.join();
    consumer.join();

    std::cout << "\nFinal Book State:\n";
    book.print_book();
    return 0;
}
