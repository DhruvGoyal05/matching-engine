#pragma once
#include <cstdint>

struct alignas(64) Order {
    uint64_t order_id;
    uint64_t price;
    uint32_t quantity;
    bool is_buy;
    bool is_market;
    int32_t next_order_idx; // Required for MemoryPool free list tracking
};
