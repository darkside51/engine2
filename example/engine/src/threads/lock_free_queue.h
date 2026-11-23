#pragma once

#include "stl/array.h"

#include <atomic>
#include <cstdint>

namespace threads {

template <typename T, uint32_t kCapacity>
class BoundedQueue {
    // capacity must be a power of two for the bitwise modulo optimization
    static_assert((kCapacity & (kCapacity - 1)) == 0, "capacity must be a power of 2");
    static constexpr uint32_t kBufferMask = kCapacity - 1;

public:
    BoundedQueue() {
        for (uint32_t i = 0u; i < kCapacity; ++i) {
            _buffer[i].seq.store(i, std::memory_order_relaxed);
        }
        _enqueueIdx.store(0u, std::memory_order_relaxed);
        _dequeueIdx.store(0u, std::memory_order_relaxed);
    }

    bool enqueue(T && data) { // returns true if successfully enqueued, false if full
        Cell * cell;
        auto pos = _enqueueIdx.load(std::memory_order_relaxed);
        
        while (true) {
            cell = &_buffer[pos & kBufferMask];
            const auto seq = cell->seq.load(std::memory_order_acquire);
            const auto diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
            
            if (diff == 0) { // cell is ready for writing. Claim the ticket.
                if (_enqueueIdx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) { // the queue is full
                return false;
            } else { // another thread beat us; reload the position counter
                pos = _enqueueIdx.load(std::memory_order_relaxed);
            }
        }

        cell->data = std::move(data);
        // signal consumers that data is available in this cell
        cell->seq.store(pos + 1u, std::memory_order_release);
        return true;
    }

    bool dequeue(T & value) { // returns true if successfully dequeued, false if empty
        Cell * cell;
        auto pos = _dequeueIdx.load(std::memory_order_relaxed);
        
        while (true) {
            cell = &_buffer[pos & kBufferMask];
            const auto seq = cell->seq.load(std::memory_order_acquire);
            const auto diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
            
            if (diff == 0) { // cell has unread data. Claim the ticket.
                if (_dequeueIdx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) { // the queue is empty
                return false;
            } else { // another thread beat us; reload the position counter
                pos = _dequeueIdx.load(std::memory_order_relaxed);
            }
        }

        value = std::move(cell->data);
        // signal producers that the cell is ready to be overwritten on the next lap
        cell->seq.store(pos + kBufferMask + 1u, std::memory_order_release);
        return true;
    }

private:
    struct Cell {
        std::atomic<uint32_t> seq;
        T data;
    };

    engine::stl::array<Cell, kCapacity> _buffer;
    
    // align variables to their own cache lines to prevent performance-killing false sharing
    alignas(64u) std::atomic<uint32_t> _enqueueIdx;
    alignas(64u) std::atomic<uint32_t> _dequeueIdx;
};
}