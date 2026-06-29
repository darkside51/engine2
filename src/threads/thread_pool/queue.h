#pragma once

#include "../../utils/system.h"

#include <atomic>
#include <cstdint>

namespace threads {
    /*
    kDefaultBoundedTaskQueueLogSize
    The default size of the bounded task queue in Log2. 
    Bounded task queue is used by each worker.
    */
    constexpr uint32_t kDefaultBoundedTaskQueueLogSize = 8u;

    /*
    kDefaultUnBoundedTaskQueueLogSize
    The default size of the unbounded task queue in Log2.
    Unbounded task queue is used by the executor.
    */
    constexpr uint32_t kDefaultUnBoundedTaskQueueLogSize = 10u;

    /*
    BoundedTaskQueue

    T data type
    LogSize the base-2 logarithm of the queue size

    class to create a lock-free bounded single-producer multiple-consumer queue

    This class implements the work-stealing queue described in the paper, 
    "Correct and Efficient Work-Stealing for Weak Memory Models,"
    available at https://www.di.ens.fr/~zappa/readings/ppopp13.pdf.

    Only the queue owner can perform pop and push operations,
    while others can steal data from the queue.
    */

    template <typename T, std::size_t LogSize = kDefaultBoundedTaskQueueLogSize>
    class BoundedTaskQueue {
        static_assert(std::is_pointer_v<T>, "T must be a pointer type");
  
        constexpr static int64_t kBufferSize = int64_t{1} << LogSize;
        constexpr static int64_t kBufferMask = (kBufferSize - 1);

        static_assert((kBufferSize >= 2) && ((kBufferSize & (kBufferSize - 1)) == 0));

        alignas(2 * kCachelineSize) std::atomic<int64_t> _top {0};
        alignas(2 * kCachelineSize) std::atomic<int64_t> _bottom {0};
        alignas(2 * kCachelineSize) std::atomic<T> _buffer[kBufferSize];

    public:
        BoundedTaskQueue() = default;
        ~BoundedTaskQueue() = default;

        bool empty() const noexcept {
            const int64_t b = _bottom.load(std::memory_order_relaxed);
            const int64_t t = _top.load(std::memory_order_relaxed);
            return b <= t;
        }

        size_t size() const noexcept {
            const int64_t b = _bottom.load(std::memory_order_relaxed);
            const int64_t t = _top.load(std::memory_order_relaxed);
            return static_cast<size_t>(b >= t ? b - t : 0);
        }

        constexpr size_t capacity() const noexcept {
            return static_cast<size_t>(kBufferSize - 1);
        }

        template <typename O>
        bool try_push(O && item) {
            const int64_t b = _bottom.load(std::memory_order_relaxed);
            const int64_t t = _top.load(std::memory_order_acquire);

            // queue is full
            if ((b - t) >= kBufferSize - 1) [[unlikely]] {
                return false;
            }
  
            _buffer[b & kBufferMask].store(std::forward<O>(item), std::memory_order_relaxed);

            std::atomic_thread_fence(std::memory_order_release);
            _bottom.store(b + 1, std::memory_order_relaxed);

            return true;
        }

        template <typename O, typename C>
        void push(O && item, C && on_full) {
            const int64_t b = _bottom.load(std::memory_order_relaxed);
            const int64_t t = _top.load(std::memory_order_acquire);

            // queue is full
            if ((b - t) >= kBufferSize - 1) [[unlikely]] {
                on_full();
                return;
            }
  
            _buffer[b & kBufferMask].store(std::forward<O>(item), std::memory_order_relaxed);

            std::atomic_thread_fence(std::memory_order_release);
            _bottom.store(b + 1, std::memory_order_relaxed);
        }

        T pop() {
            const int64_t b = _bottom.load(std::memory_order_relaxed) - 1;
            _bottom.store(b, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            int64_t t = _top.load(std::memory_order_relaxed);

            T item {nullptr};

            if (t <= b) {
                item = _buffer[b & kBufferMask].load(std::memory_order_relaxed);
                if (t == b) {
                    // the last item just got stolen
                    if (!_top.compare_exchange_strong(t, t + 1, 
                                        std::memory_order_seq_cst, 
                                        std::memory_order_relaxed)) {
                        item = nullptr;
                    }
                    _bottom.store(b + 1, std::memory_order_relaxed);
                }
            } else {
                _bottom.store(b + 1, std::memory_order_relaxed);
            }

            return item;
        }

        T steal() {
            int64_t t = _top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            const int64_t b = _bottom.load(std::memory_order_acquire);
  
            T item {nullptr};

            if (t < b) {
                item = _buffer[t & kBufferMask].load(std::memory_order_relaxed);
                if (!_top.compare_exchange_strong(t, t + 1,
                                    std::memory_order_seq_cst,
                                    std::memory_order_relaxed)) {
                    return nullptr;
                }
            }

            return item;
        }
    };

    /*
    UnboundedTaskQueue

    T data type (must be a pointer type)

    class to create a lock-free unbounded single-producer multiple-consumer queue

    This class implements the work-stealing queue described in the paper,
    <a href="https://www.di.ens.fr/~zappa/readings/ppopp13.pdf">Correct and Efficient Work-Stealing for Weak Memory Models</a>.

    Only the queue owner can perform pop and push operations,
    while others can steal data from the queue simultaneously.
    */
    template <typename T>
    class UnboundedTaskQueue {
        static_assert(std::is_pointer_v<T>, "T must be a pointer type");

        struct Array {

        int64_t c;
        int64_t m;
        std::atomic<T>* s;

        explicit Array(int64_t c) : c {c}, m {c - 1}, s {new std::atomic<T>[static_cast<size_t>(c)]} {}

        ~Array() {
            delete [] s;
        }

        int64_t capacity() const noexcept {
            return c;
        }

        void push(int64_t i, T o) noexcept {
            s[i & m].store(o, std::memory_order_relaxed);
        }

        T pop(int64_t i) noexcept {
            return s[i & m].load(std::memory_order_relaxed);
        }

        Array* resize(int64_t b, int64_t t) {
            Array* ptr = new Array {2 * c};
            for (int64_t i = t; i != b; ++i) {
                ptr->push(i, pop(i));
            }
            return ptr;
        }
  };

    // Doubling the alignment by 2 seems to generate the most
    // decent performance.
    alignas(2 * kCachelineSize) std::atomic<int64_t> _top;
    alignas(2 * kCachelineSize) std::atomic<int64_t> _bottom;
    std::atomic<Array*> _array;
    std::vector<Array*> _garbage;

  public:
    explicit UnboundedTaskQueue(int64_t LogSize = kDefaultUnBoundedTaskQueueLogSize) {
        _top.store(0, std::memory_order_relaxed);
        _bottom.store(0, std::memory_order_relaxed);
        _array.store(new Array{(int64_t{1} << LogSize)}, std::memory_order_relaxed);
        _garbage.reserve(32u);
    }

    ~UnboundedTaskQueue() {
        for (auto a : _garbage) {
            delete a;
        }
        delete _array.load();
    }

    bool empty() const noexcept {
        const int64_t b = _bottom.load(std::memory_order_relaxed);
        const int64_t t = _top.load(std::memory_order_relaxed);
        return (b <= t);
    }

    size_t size() const noexcept {
        const int64_t b = _bottom.load(std::memory_order_relaxed);
        const int64_t t = _top.load(std::memory_order_relaxed);
        return static_cast<size_t>(b >= t ? b - t : 0);
    }

    int64_t capacity() const noexcept {
        return _array.load(std::memory_order_relaxed)->capacity();
    }

    void push(T item) {
        const int64_t b = _bottom.load(std::memory_order_relaxed);
        const int64_t t = _top.load(std::memory_order_acquire);
        Array* a = _array.load(std::memory_order_relaxed);

        // queue is full
        if (a->capacity() - 1 < (b - t)) [[unlikely]] {
            a = resize_array(a, b, t);
        }

        a->push(b, item);
        std::atomic_thread_fence(std::memory_order_release);
        _bottom.store(b + 1, std::memory_order_relaxed);
    }

    T pop() {
        const int64_t b = _bottom.load(std::memory_order_relaxed) - 1;
        Array* a = _array.load(std::memory_order_relaxed);
        _bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t t = _top.load(std::memory_order_relaxed);

        T item {nullptr};

        if (t <= b) {
            item = a->pop(b);
            if (t == b) {
                // the last item just got stolen
                if (!_top.compare_exchange_strong(t, t+1,
                                                std::memory_order_seq_cst,
                                                std::memory_order_relaxed)) {
                    item = nullptr;
                }
                _bottom.store(b + 1, std::memory_order_relaxed);
            }
        } else {
            _bottom.store(b + 1, std::memory_order_relaxed);
        }

        return item;
    }

    T steal() {
        int64_t t = _top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const int64_t b = _bottom.load(std::memory_order_acquire);

        T item {nullptr};

        if (t < b) {
            Array* a = _array.load(std::memory_order_consume);
            item = a->pop(t);
            if (!_top.compare_exchange_strong(t, t+1,
                                        std::memory_order_seq_cst,
                                        std::memory_order_relaxed)) {
                return nullptr;
            }
        }

        return item;
    }

  private:
    Array* resize_array(Array* a, int64_t b, int64_t t) {
        Array* tmp = a->resize(b, t);
        _garbage.push_back(a);
        std::swap(a, tmp);
        _array.store(a, std::memory_order_release);
        // Note: the original paper using relaxed causes t-san to complain
        //_array.store(a, std::memory_order_relaxed);
        return a;
    }
};
}