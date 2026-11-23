#pragma once

#include "notifier.h"
#include "queue.h"

#include <cstdint>
#include <random>

namespace threads {

    class ThreadPool;
    class Task;

    class PoolWorker {
        friend class ThreadPool;
        friend class PoolWorkerView;
    public:

    // queries the worker id associated with its parent executor
    // A worker id is a unsigned integer in the range <tt>[0, N)</tt>,
    // where N is the number of workers spawned at the construction
    // time of the executor.
    inline std::size_t id() const { return _id; }

    // queries the size of the queue (i.e., number of pending tasks to run) associated with the worker
    inline std::size_t queue_size() const { return _wsq.size(); }

    //queries the current capacity of the queue
    inline std::size_t queue_capacity() const { return static_cast<std::size_t>(_wsq.capacity()); }

    private:
        std::size_t _id;
        std::size_t _vtm;
        ThreadPool* _executor {nullptr};
        std::default_random_engine _rdgen { std::random_device{}() };
        BoundedTaskQueue<Task*> _wsq;
        Task* _cache {nullptr};

        Notifier::Waiter* _waiter;
    };

    namespace for_each_thread {
        inline thread_local PoolWorker* worker {nullptr};
    }

    class PoolWorkerView {
        friend class ExecutionContext;

    public:
        inline std::size_t id() const noexcept { return _worker._id; }
        inline std::size_t queue_size() const noexcept { return _worker.queue_size(); }
        inline std::size_t queue_capacity() const noexcept { return _worker.queue_capacity(); }

    private:
        PoolWorkerView(const PoolWorker& w) : _worker(w) {}
        PoolWorkerView(const PoolWorkerView&) = default;

        const PoolWorker& _worker;
    };

}