#pragma once

#include "../../utils/os.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <vector>

namespace threads {

    class Notifier {
        // This requires 64-bit
        static_assert(sizeof(int) == 4u, "bad platform");
        static_assert(sizeof(uint32_t) == 4u, "bad platform");
        static_assert(sizeof(uint64_t) == 8u, "bad platform");
        static_assert(sizeof(std::atomic<uint64_t>) == 8u, "bad platform");

        friend class ThreadPool;

        static constexpr uint64_t WAITER_INC  {1u};
        static constexpr uint64_t EPOCH_SHIFT {32u};
        static constexpr uint64_t EPOCH_INC   {uint64_t{1u} << EPOCH_SHIFT};
        static constexpr uint64_t WAITER_MASK {EPOCH_INC - 1u};

    public:
        struct Waiter {
            alignas (2 * kCachelineSize) uint32_t epoch;
        };

        Notifier(std::size_t N) noexcept : _state(0), _waiters(N) {}
        ~Notifier() { assert((_state.load() & WAITER_MASK) == 0u); } 

        inline void notify_one() noexcept {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if ((_state.load(std::memory_order_acquire) & WAITER_MASK) != 0) {
                _state.fetch_add(EPOCH_INC, std::memory_order_release);
                _state.notify_one(); 
            }
        }

        inline void notify_all() noexcept {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if ((_state.load(std::memory_order_acquire) & WAITER_MASK) != 0u) {
                _state.fetch_add(EPOCH_INC, std::memory_order_release);
                _state.notify_all(); 
            }
        }

        inline void notify_n(std::size_t n) noexcept {
            if (n >= _waiters.size()) {
                notify_all();
            } else {
                for (auto k = 0u; k < n; ++k) {
                    notify_one();
                }
            }
        }

        inline void prepare_wait(Waiter * waiter) noexcept {
            waiter->epoch = _state.fetch_add(WAITER_INC, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }

        inline void cancel_wait(Waiter *) noexcept {
            _state.fetch_sub(WAITER_INC, std::memory_order_relaxed);
        }

        inline void commit_wait(Waiter * waiter) noexcept {
            _state.wait(waiter->epoch, std::memory_order_seq_cst);
            _state.fetch_sub(WAITER_INC, std::memory_order_relaxed);
        }

        inline std::size_t size() const noexcept {
            return _waiters.size();
        }

    private:
        Notifier(const Notifier &) = delete;
        Notifier(Notifier &&) = delete;
        Notifier & operator=(const Notifier &) = delete;
        Notifier & operator=(Notifier &&) = delete;

        // _state stores the epoch in the most significant 32 bits and the
        // waiter count in the least significant 32 bits.
        std::atomic<uint64_t> _state;
        std::vector<Waiter> _waiters;
    };
}