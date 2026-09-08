#pragma once
#include "Order.hpp"
#include <vector>
#include <cstdint>

template <size_t N>
class MemoryPool {
private:
    std::vector<Order> pool_;
    int32_t free_list_head_;

public:
    MemoryPool() : pool_(N) {
        // Initialize the free list pointers
        for (size_t i = 0; i < N - 1; ++i) {
            pool_[i].next_order_idx = static_cast<int32_t>(i + 1);
        }
        pool_[N - 1].next_order_idx = -1;
        free_list_head_ = 0;
    }

    Order* allocate() {
        if (free_list_head_ == -1) return nullptr; // Pool exhausted
        int32_t idx = free_list_head_;
        free_list_head_ = pool_[idx].next_order_idx;
        return &pool_[idx];
    }

    void deallocate(Order* order) {
        int32_t idx = static_cast<int32_t>(order - &pool_[0]);
        pool_[idx].next_order_idx = free_list_head_;
        free_list_head_ = idx;
    }
};
