#pragma once
#include <atomic>
#include <vector>
#include <optional>

template <typename T, size_t Capacity>
class SPSCQueue {
private:
    struct Cell {
        std::atomic<size_t> sequence;
        T data;
    };

    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    Cell buffer_[Capacity];
    const size_t mask_ = Capacity - 1;

    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};

public:
    SPSCQueue() {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    bool push(const T& data) {
        Cell* cell;
        size_t head = head_.load(std::memory_order_relaxed);
        
        for (;;) {
            cell = &buffer_[head & mask_];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)head;
            
            if (dif == 0) {
                if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (dif < 0) {
                return false; // Queue is full
            } else {
                head = head_.load(std::memory_order_relaxed);
            }
        }

        cell->data = data;
        cell->sequence.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& data) {
        Cell* cell;
        size_t tail = tail_.load(std::memory_order_relaxed);
        
        for (;;) {
            cell = &buffer_[tail & mask_];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)(tail + 1);
            
            if (dif == 0) {
                if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (dif < 0) {
                return false; // Queue is empty
            } else {
                tail = tail_.load(std::memory_order_relaxed);
            }
        }

        data = cell->data;
        cell->sequence.store(tail + mask_ + 1, std::memory_order_release);
        return true;
    }
};
