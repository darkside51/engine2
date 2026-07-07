#pragma once

// #include "threads/threads.h"

#include <atomic>
#include <cstdint>
#include <functional>

namespace threads {
    // using Task = WorkerTask;

    class Task {
        friend class ThreadPool;
        // using Work = std::function<void()>;
        using Work = std::move_only_function<void()>;

    public:
        Task() : _work(nullptr) {}

        template <typename... Args>
        explicit Task(Args &&... args) : _work(std::forward<Args>(args)...) {};
        ~Task() = default;

        Task(Task && t) noexcept : _work(std::move(t._work)) {};
        Task(const Task &) = delete;

        inline void operator()() { _work(); }
        // inline void operator()() const { _work(); }

    private:
        Work _work;
    };
}