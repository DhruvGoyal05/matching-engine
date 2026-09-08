#pragma once
#include <cstdint>

struct alignas(64) Order {
    uint64_t order_id;
    uint64_t price;
    uint32_t quantity;
    uint32_t trader_id;
    bool is_buy;

    int32_t next_order_idx;
    int32_t prev_order_idx;
};
