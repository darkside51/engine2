#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace threads {

    class Task {
        friend class ThreadPool;
        using Work = std::function<void()>;

    public:
        Task() : _work(nullptr) {}

        template <typename... Args>
        explicit Task(Args &&... args) : _work(std::forward<Args>(args)...) {};
        ~Task() = default;

        Task(Task && t) noexcept : _work(std::move(t._work)) {};
        Task(const Task &) = delete;

        inline void operator()() const { _work(); }

    private:
        Work _work;
    };
}