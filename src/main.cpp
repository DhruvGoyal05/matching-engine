#include "OrderBook.hpp"
#include "SPSCQueue.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>

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
    book.set_trade_callback([](uint64_t, uint64_t, uint64_t, uint32_t) {
        // Zero-I/O in production
    });

    constexpr int NUM_ORDERS = 10000;
    SPSCQueue<OrderCommand, 16384> queue;
    std::atomic<bool> producer_done{false};
    std::vector<double> latencies_ns;
    latencies_ns.reserve(NUM_ORDERS);

    std::thread consumer([&]() {
        OrderCommand cmd;
        int processed = 0;
        while (processed < NUM_ORDERS) {
            if (queue.pop(cmd)) {
                auto start = std::chrono::high_resolution_clock::now();
                
                OrderType type = cmd.is_market ? OrderType::MARKET : OrderType::LIMIT;
                book.add_order(cmd.id, cmd.price, cmd.qty, cmd.is_buy, type);
                
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::nano> elapsed = end - start;
                latencies_ns.push_back(elapsed.count());
                processed++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread producer([&]() {
        for (int i = 0; i < NUM_ORDERS; ++i) {
            bool is_buy = (i % 2 == 0);
            uint64_t price = 10050 + (i % 5);
            queue.push({static_cast<uint64_t>(i + 1), price, 10, is_buy, false, false});
        }
        producer_done.store(true, std::memory_order_relaxed);
    });

    producer.join();
    consumer.join();

    std::sort(latencies_ns.begin(), latencies_ns.end());
    
    double min_ns = latencies_ns.front();
    double max_ns = latencies_ns.back();
    double p50_ns = latencies_ns[NUM_ORDERS * 0.50];
    double p99_ns = latencies_ns[NUM_ORDERS * 0.99];
    
    double sum = std::accumulate(latencies_ns.begin(), latencies_ns.end(), 0.0);
    double avg_ns = sum / NUM_ORDERS;

    std::cout << "--- OPTIMIZED BENCHMARK RESULTS (" << NUM_ORDERS << " Orders) ---\n";
    std::cout << "Avg Latency: " << avg_ns << " ns\n";
    std::cout << "Min Latency: " << min_ns << " ns\n";
    std::cout << "p50 Latency: " << p50_ns << " ns\n";
    std::cout << "p99 Latency: " << p99_ns << " ns\n";
    std::cout << "Max Latency: " << max_ns << " ns\n";
    std::cout << "---------------------------------------------------\n";

    return 0;
}
