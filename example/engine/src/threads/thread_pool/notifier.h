#pragma once

#include "core/system.h"
#include "stl/vector.h"

#include <atomic>
#include <cassert>
#include <cstdint>

namespace threads {
    class NotifierV1 {
        // This requires 64-bit
        static_assert(sizeof(int) == 4, "bad platform");
        static_assert(sizeof(uint32_t) == 4, "bad platform");
        static_assert(sizeof(uint64_t) == 8, "bad platform");
        static_assert(sizeof(std::atomic<uint64_t>) == 8, "bad platform");

        friend class ThreadPool;

    public:
        struct Waiter {
            alignas (2 * kCachelineSize) uint32_t epoch;
        };

        NotifierV1(size_t N) noexcept : _state(0), _waiters(N) {}
        ~NotifierV1() { assert((_state.load() & WAITER_MASK) == 0); } 

        void notify_one() noexcept;
        void notify_all() noexcept;
        void notify_n(size_t n) noexcept;
        void prepare_wait(Waiter*) noexcept;
        void cancel_wait(Waiter*) noexcept;
        void commit_wait(Waiter*) noexcept;
        size_t size() const noexcept;

    private:
        NotifierV1(const NotifierV1 &) = delete;
        NotifierV1(NotifierV1 &&) = delete;
        NotifierV1 & operator=(const NotifierV1 &) = delete;
        NotifierV1 & operator=(NotifierV1 &&) = delete;

        // _state stores the epoch in the most significant 32 bits and the
        // waiter count in the least significant 32 bits.
        std::atomic<uint64_t> _state;
        engine::stl::vector<Waiter> _waiters;

        static constexpr uint64_t WAITER_INC  {1};
        static constexpr size_t   EPOCH_SHIFT {32};
        static constexpr uint64_t EPOCH_INC   {uint64_t(1) << EPOCH_SHIFT};
        static constexpr uint64_t WAITER_MASK {EPOCH_INC - 1};
    };

    inline void NotifierV1::notify_one() noexcept {
        uint64_t prev = _state.fetch_add(EPOCH_INC, std::memory_order_acq_rel);
        if ((prev & WAITER_MASK))  { // has waiter (typically unlikely)
            _state.notify_one();
        }
    }

    inline void NotifierV1::notify_all() noexcept {
        uint64_t prev = _state.fetch_add(EPOCH_INC, std::memory_order_acq_rel);
        if ((prev & WAITER_MASK))  { // has waiter (typically unlikely)
            _state.notify_all();
        }
    }
        
    inline void NotifierV1::notify_n(size_t n) noexcept {
        if (n >= _waiters.size()) {
            notify_all();
        } else {
            for (size_t k = 0; k < n; ++k) {
                notify_one();
            }
        }
    }

    inline size_t NotifierV1::size() const noexcept {
        return _waiters.size();
    }

    inline void NotifierV1::prepare_wait(Waiter* waiter) noexcept {
        uint64_t prev = _state.fetch_add(WAITER_INC, std::memory_order_acq_rel);
        waiter->epoch = (prev >> EPOCH_SHIFT);
    }

    inline void NotifierV1::cancel_wait(Waiter*) noexcept {
        // memory_order_relaxed would suffice for correctness, but the faster
        // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
        // (and thus system calls).
        _state.fetch_sub(WAITER_INC, std::memory_order_seq_cst);
    }

    inline void NotifierV1::commit_wait(Waiter* waiter) noexcept {
        uint64_t prev = _state.load(std::memory_order_acquire);
        while((prev >> EPOCH_SHIFT) == waiter->epoch) {
            _state.wait(prev, std::memory_order_acquire); 
            prev = _state.load(std::memory_order_acquire);
        }
        // memory_order_relaxed would suffice for correctness, but the faster
        // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
        // (and thus system calls)
        _state.fetch_sub(WAITER_INC, std::memory_order_seq_cst);
    }

    class NotifierV2 { // commit_wait incorrect, threads no sleeps :(
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

        NotifierV2(std::size_t N) noexcept : _state(0), _waiters(N) {}
        ~NotifierV2() { assert((_state.load() & WAITER_MASK) == 0u); } 

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
        NotifierV2(const NotifierV2 &) = delete;
        NotifierV2(NotifierV2 &&) = delete;
        NotifierV2 & operator=(const NotifierV2 &) = delete;
        NotifierV2 & operator=(NotifierV2 &&) = delete;

        // _state stores the epoch in the most significant 32 bits and the
        // waiter count in the least significant 32 bits.
        std::atomic<uint64_t> _state;
        engine::stl::vector<Waiter> _waiters;
    };


    // none blicking notifier v2
    class NonblockingNotifierV2 {
        friend class ThreadPool;

    public:
        struct Waiter {
            alignas (2 * kCachelineSize) std::atomic<uint64_t> next;
            uint64_t epoch;
            enum : unsigned {
                kNotSignaled = 0,
                kWaiting,
                kSignaled,
            };

            #ifdef __cpp_lib_atomic_wait
            std::atomic<unsigned> state {0};
            #else
            std::mutex mu;
            std::condition_variable cv;
            unsigned state;
            #endif
        };

        explicit NonblockingNotifierV2(size_t N) : _state(kStackMask), _waiters(N) {
            assert(N < ((1 << kWaiterBits) - 1));
        }

        ~NonblockingNotifierV2() {
            // Ensure there are no waiters.
            assert(_state.load() == kStackMask);
        }

        // prepare_wait prepares for waiting.
        // After calling prepare_wait, the thread must re-check the wait predicate
        // and then call either cancel_wait or commit_wait.
        void prepare_wait(Waiter*) {
            uint64_t state = _state.load(std::memory_order_relaxed);
            for (;;) {
                //_check_state(state);
                uint64_t newstate = state + kWaiterInc;
                //_check_state(newstate);
                if (_state.compare_exchange_weak(state, newstate, std::memory_order_seq_cst)) return;
            }
        }

        // commit_wait commits waiting after prepare_wait.
        void commit_wait(Waiter* w) {
            #ifdef __cpp_lib_atomic_wait
            w->state.store(Waiter::kNotSignaled, std::memory_order_relaxed);
            #else
            w->state = Waiter::kNotSignaled;
            #endif
            const uint64_t me = (w - &_waiters[0]) | w->epoch;
            uint64_t state = _state.load(std::memory_order_seq_cst);
            for (;;) {
                //_check_state(state, true);
                uint64_t newstate;
                if ((state & kSignalMask) != 0) {
                // Consume the signal and return immediately.
                newstate = state - kWaiterInc - kSignalInc;
                } else {
                // Remove this thread from pre-wait counter and add to the waiter stack.
                newstate = ((state & kWaiterMask) - kWaiterInc) | me;
                w->next.store(state & (kStackMask | kEpochMask), std::memory_order_relaxed);
                }
                //_check_state(newstate);
                if (_state.compare_exchange_weak(state, newstate, std::memory_order_acq_rel)) {
                if ((state & kSignalMask) == 0) {
                    w->epoch += kEpochInc;
                    _park(w);
                }
                return;
                }
            }
        }

        // cancel_wait cancels effects of the previous prepare_wait call.
        void cancel_wait(Waiter*) {
            uint64_t state = _state.load(std::memory_order_relaxed);
            for (;;) {
                //_check_state(state, true);
                uint64_t newstate = state - kWaiterInc;
                // We don't know if the thread was also notified or not,
                // so we should not consume a signal unconditionally.
                // Only if number of waiters is equal to number of signals,
                // we know that the thread was notified and we must take away the signal.
                if (((state & kWaiterMask) >> kWaiterShift) == ((state & kSignalMask) >> kSignalShift)) newstate -= kSignalInc;
                //_check_state(newstate);
                if (_state.compare_exchange_weak(state, newstate, std::memory_order_acq_rel)) return;
            }
        }

        void notify_one() {
            _notify<false>();
        }

        void notify_all() {
            _notify<true>();
        }

        // notify n workers
        void notify_n(size_t n) {
            if (n >= _waiters.size()) {
                _notify<true>();
            } else {
                for (size_t k = 0; k < n; ++k) {
                    _notify<false>();
                }
            }
        }

        size_t size() const {
            return _waiters.size();
        }

    private:
        // State_ layout:
        // - low kWaiterBits is a stack of waiters committed wait
        //   (indexes in _waiters array are used as stack elements,
        //   kStackMask means empty stack).
        // - next kWaiterBits is count of waiters in prewait state.
        // - next kWaiterBits is count of pending signals.
        // - remaining bits are ABA counter for the stack.
        //   (stored in Waiter node and incremented on push).
        static const uint64_t kWaiterBits = 14;
        static const uint64_t kStackMask = (1ull << kWaiterBits) - 1;
        static const uint64_t kWaiterShift = kWaiterBits;
        static const uint64_t kWaiterMask = ((1ull << kWaiterBits) - 1) << kWaiterShift;
        static const uint64_t kWaiterInc = 1ull << kWaiterShift;
        static const uint64_t kSignalShift = 2 * kWaiterBits;
        static const uint64_t kSignalMask = ((1ull << kWaiterBits) - 1) << kSignalShift;
        static const uint64_t kSignalInc = 1ull << kSignalShift;
        static const uint64_t kEpochShift = 3 * kWaiterBits;
        static const uint64_t kEpochBits = 64 - kEpochShift;
        static const uint64_t kEpochMask = ((1ull << kEpochBits) - 1) << kEpochShift;
        static const uint64_t kEpochInc = 1ull << kEpochShift;

        static_assert(kEpochBits >= 20, "not enough bits to prevent ABA problem");

        std::atomic<uint64_t> _state;
        engine::stl::vector<Waiter> _waiters;

        void _park(Waiter* w) {
            #ifdef __cpp_lib_atomic_wait
            unsigned target = Waiter::kNotSignaled;
            if (w->state.compare_exchange_strong(target, Waiter::kWaiting,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
                w->state.wait(Waiter::kWaiting, std::memory_order_relaxed);
            }
            #else
            std::unique_lock<std::mutex> lock(w->mu);
            while (w->state != Waiter::kSignaled) {
                w->state = Waiter::kWaiting;
                w->cv.wait(lock);
            }
            #endif
        }

        void _unpark(Waiter* w) {
            for (Waiter* next; w; w = next) {
                uint64_t wnext = w->next.load(std::memory_order_relaxed) & kStackMask;
                next = (wnext == kStackMask) ? nullptr : &_waiters[static_cast<size_t>(wnext)];
            #ifdef __cpp_lib_atomic_wait
                if(w->state.exchange(Waiter::kSignaled, std::memory_order_relaxed) == 
                    Waiter::kWaiting) {
                w->state.notify_one();
                }
            #else      
                unsigned state;
                {
                    std::unique_lock<std::mutex> lock(w->mu);
                    state = w->state;
                    w->state = Waiter::kSignaled;
                }
                // Avoid notifying if it wasn't waiting.
                if (state == Waiter::kWaiting) w->cv.notify_one();
            #endif
            }
        }

        // Notify wakes one or all waiting threads.
        // Must be called after changing the associated wait predicate.
        template <bool notifyAll>
        void _notify() {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            uint64_t state = _state.load(std::memory_order_acquire);
            for (;;) {
                //_check_state(state);
                const uint64_t waiters = (state & kWaiterMask) >> kWaiterShift;
                const uint64_t signals = (state & kSignalMask) >> kSignalShift;
                // Easy case: no waiters.
                if ((state & kStackMask) == kStackMask && waiters == signals) return;
                uint64_t newstate;
                if (notifyAll) {
                    // Empty wait stack and set signal to number of pre-wait threads.
                    newstate = (state & kWaiterMask) | (waiters << kSignalShift) | kStackMask;
                } else if (signals < waiters) {
                    // There is a thread in pre-wait state, unblock it.
                    newstate = state + kSignalInc;
                } else {
                    // Pop a waiter from list and unpark it.
                    Waiter* w = &_waiters[state & kStackMask];
                    uint64_t next = w->next.load(std::memory_order_relaxed);
                    newstate = (state & (kWaiterMask | kSignalMask)) | next;
                }
                //_check_state(newstate);
                if (_state.compare_exchange_weak(state, newstate, std::memory_order_acq_rel)) {
                    if (!notifyAll && (signals < waiters)) return;  // unblocked pre-wait thread
                    if ((state & kStackMask) == kStackMask) return;
                    Waiter* w = &_waiters[state & kStackMask];
                    if (!notifyAll) w->next.store(kStackMask, std::memory_order_relaxed);
                    _unpark(w);
                    return;
                }
            }
        }

        NonblockingNotifierV2(const NonblockingNotifierV2 &) = delete;
        void operator= (const NonblockingNotifierV2 &) = delete;
    };

    // using Notifier = NotifierV2;
    using Notifier = NonblockingNotifierV2;
}