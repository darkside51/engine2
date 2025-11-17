#pragma once

#include "../threads.h"
#include "task.h"
#include "worker.h"
#include "free_list.h"
#include "notifier.h"

#include <atomic>
#include <cassert>
#include <latch>
#include <list>
#include <mutex>
#include <stop_token>
#include <thread>

#define ENABLE_TASK_POOL

#ifdef ENABLE_TASK_POOL
#include "../../allocators/pool_allocator_chain.h"
#endif

namespace threads {
    class ThreadPool {
    public:
    #ifdef ENABLE_TASK_POOL
        explicit ThreadPool(size_t n = std::thread::hardware_concurrency(), size_t poolCapacity = 1024u, size_t poolsCount = 64u) :
            _maxSteals ((n + 1u) << 1u),
            _threads    (n),
            _workers    (n),
            _notifier   (n),
            _latch      (n + 1u),
            _freelist   (n),
            _taskPool {poolCapacity, poolsCount} {
                assert(n > 0u);
                spawn(n);
        }
    #else
    explicit ThreadPool(size_t n = std::thread::hardware_concurrency()) :
            _maxSteals ((n + 1u) << 1u),
            _threads    (n),
            _workers    (n),
            _notifier   (n),
            _latch      (n + 1u),
            _freelist   (n) {
                assert(n > 0u);
                spawn(n);
        }
    #endif

        ~ThreadPool() noexcept {
            cancelTasks();
            stop();
        }

        template <typename... Args>
        inline void execute(Args &&... args) {
            run(createTask(std::forward<Args>(args)...));
        }

        inline void run(Task * task) {
            auto * worker = for_each_thread::worker;
            worker ? schedule(*worker, task) : schedule(task);
        }

        inline void stop() noexcept {
            _done.test_and_set(std::memory_order_relaxed);
            _notifier.notify_all();
            for (auto & thread : _threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }

        inline void cancelTasks() noexcept {
            _cancelTasks.test_and_set(std::memory_order_relaxed);
        }

        inline size_t numWorkers() const noexcept { return _workers.size(); }

        inline int this_worker_id() const noexcept {
            auto * w = for_each_thread::worker;
            return (w && w->_executor == this) ? w->_id : -1;
        }

        template <typename... Args>
        inline Task * createTask(Args &&... args) {
        #ifdef ENABLE_TASK_POOL
            return _taskPool.create<Task>(std::forward<Args>(args)...);
        #else
            return new Task(std::forward<Args>(args)...);
        #endif
        }

        inline void destroyTask(Task * ptr) noexcept {
        #ifdef ENABLE_TASK_POOL
            _taskPool.destroy(ptr);
        #else
            delete ptr;
        #endif
        }

    private:
        void spawn(size_t n) noexcept {
            for (size_t id = 0u; id < n; ++id) {
                _workers[id]._id = id;
                _workers[id]._vtm = id;
                _workers[id]._executor = this;
                _workers[id]._waiter = &_notifier._waiters[id];
                _threads[id] = std::thread([&, &w = _workers[id]] () {
                    for_each_thread::worker = &w;
                    _latch.arrive_and_wait();  // synchronize with the main thread

                    Task * t = nullptr;
      
                    while (true) {
                        // execute the tasks.
                        exploit_task(w, t);

                        // wait for tasks
                        if (wait_for_task(w, t) == false) {
                            break;
                        }
                    }
                });
            }

            _latch.arrive_and_wait();
        }

        inline void exploit_task(PoolWorker & w, Task *& t) {
            while (t) {
                invoke(w, t);
                t = w._wsq.pop();
            }
        }

        inline void invoke(PoolWorker & w, Task *& t) {
            // no need to do other things if the topology is cancelled
            if (!_cancelTasks.test(std::memory_order_relaxed)) {
                (*t)();
            }

            //~~~~~~~~~~~~
            destroyTask(t);
        }

        inline bool wait_for_task(PoolWorker & w, Task *& t) {
            explore_task:
            
            explore_task(w, t);
            if (t) {
                return true;
            }
            
            // ---- 2PC guard ----
            _notifier.prepare_wait(w._waiter);

            if (!_freelist.empty()) {
                _notifier.cancel_wait(w._waiter);
                w._vtm = w._id;
                goto explore_task;
            }

            if (_done.test(std::memory_order_relaxed)) {
                _notifier.cancel_wait(w._waiter);
                _notifier.notify_all();
                return false;
            }

            // We need to use index-based scanning to avoid data race
            // with _spawn which may initialize a worker at the same time.
            for (size_t vtm = 0u; vtm < _workers.size(); ++vtm) {
                if (!_workers[vtm]._wsq.empty()) {
                    _notifier.cancel_wait(w._waiter);
                    w._vtm = vtm;
                    goto explore_task;
                }
            }
  
            // Now I really need to relinquish my self to others
            _notifier.commit_wait(w._waiter);
            goto explore_task;
        }

        inline void explore_task(PoolWorker & w, Task *& t) {
            size_t steals = 0u;
            size_t yields = 0u;
            constexpr size_t kMaxYields = 100u;

            std::uniform_int_distribution<size_t> rdvtm(0u, _workers.size() - 1u);

            // Here, we write do-while to make the worker steal at once
            // from the assigned victim.
            do {
                t = (w._id == w._vtm) ? _freelist.steal(w._id) : _workers[w._vtm]._wsq.steal();
                if (t) {
                    break;
                }

                if (steals++ > _maxSteals) {
                    std::this_thread::yield();
                    if (yields++ > kMaxYields) {
                        break;
                    }
                }

                w._vtm = rdvtm(w._rdgen);
            } while (!_done.test(std::memory_order_relaxed));
        }

        inline void schedule(PoolWorker & w, Task * t) {
            if (w._executor == this) {
                w._wsq.push(t, [&](){ _freelist.push(w._id, t); });
                _notifier.notify_one();
                return;
            }
  
            _freelist.push(t);
            _notifier.notify_one();
        }

        inline void schedule(Task * t) {
            _freelist.push(t);
            _notifier.notify_one();
        }

        const size_t _maxSteals;

        std::vector<std::thread> _threads;
        std::vector<PoolWorker> _workers;

        Notifier _notifier;

        std::latch _latch;
        std::atomic_flag _done = ATOMIC_FLAG_INIT;
        std::atomic_flag _cancelTasks = ATOMIC_FLAG_INIT;

        Freelist<Task*> _freelist;

    #ifdef ENABLE_TASK_POOL
        using PoolAllocator = allocators::PoolAllocatorChain<sizeof(Task), alignof(Task), true, true>;
        PoolAllocator _taskPool{1024u, 64u};
    #endif
    };

}