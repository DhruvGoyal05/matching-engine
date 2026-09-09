#pragma once
#include <cstdint>

struct Order {
    uint64_t order_id;
    uint64_t price;
    uint32_t quantity;
    bool is_buy;
    bool is_market;
    
    // Used by MemoryPool to manage the free list
    int32_t next_order_idx = -1; 
    
    // Intrusive Linked List Pointers (Zero-allocation lists)
    Order* next = nullptr;
    Order* prev = nullptr;
};
