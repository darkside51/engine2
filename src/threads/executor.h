#pragma once

#include "threads.h"

#include <cassert>
#include <chrono>
#include <functional>

namespace threads {

    class Executor {
    public:
        ~Executor() {
            stop();
        }

        inline void stop() noexcept {
            if (!_stop.test_and_set(std::memory_order_release)) {
                isRunning() ? pause() : resume();
                if (_thread.joinable()) {
                    _thread.join();
                }
                printf("worker stopped\n");
            }
        }

        inline void pause() noexcept {
            if (!_running.test(std::memory_order_relaxed)) { 
                return;
            }
            _running.clear(std::memory_order_relaxed);
            waitPaused();
            printf("worker paused\n");
        }

        inline void resume() noexcept {
            if (!_running.test_and_set(std::memory_order_relaxed)) {
                _running.notify_one();
            }
        }

        inline std::thread::id getThreadId() const noexcept {
            return _threadId;
        }

        inline void emplaceTasks(WorkerTasks && tasks) {
            SpinLock lock(_lockTasks);
            std::move(tasks.begin(), tasks.end(), std::back_inserter(_tasks));
        }

        void performTasks() {
            assert(std::this_thread::get_id() == _threadId);
            WorkerTasks tasks;
            {
                SpinLock lock(_lockTasks);
                tasks = std::move(_tasks);
            }

            for (auto & t : tasks) {
                t();
            }
        }

    protected:
         inline void waitPaused() noexcept {
            _paused.wait(false, std::memory_order_relaxed);
            _paused.clear();
        }

        inline void waitResumed() noexcept {
            _running.wait(false, std::memory_order_relaxed);
        }

        [[nodiscard]] inline bool isRunning() const noexcept { return _running.test(std::memory_order_relaxed); }
        [[nodiscard]] inline bool isStopped() const noexcept { return _stop.test(std::memory_order_acquire); }

        std::thread _thread;
        std::thread::id _threadId;
        std::atomic_flag _running;
        std::atomic_flag _paused;
        std::atomic_flag _stop;

        std::atomic_flag _lockTasks;
        WorkerTasks _tasks;
    };

    template <typename T>
    class ExecutorImpl : public Executor {
    public:
        ExecutorImpl(T && job, float frequency) : _job(std::move(job)), _frequency(frequency) {}

        inline void run() {
            _running.test_and_set(std::memory_order_relaxed);
            _time = std::chrono::steady_clock::now();
            _thread = std::thread(&ExecutorImpl::loop, this);
        }

    private:
        void loop() {
            _threadId = std::this_thread::get_id();

            while (!isStopped()) {
                while (isRunning()) {
                    const auto time = std::chrono::steady_clock::now();
                    const std::chrono::duration<float> duration = time - _time; // as default in seconds
				    const auto delta = duration.count();

                    if (auto const t = _frequency - delta; t > 0.0f) {
                        std::this_thread::sleep_for(std::chrono::duration<float>(t));
                        continue;
                    }

                    _time = time;

                    _job(*this, delta);
                }

                if (!_paused.test_and_set(std::memory_order_relaxed)) {
                    _paused.notify_one();
                }

                if (!isStopped()) {
                    _time = std::chrono::steady_clock::now();
                    waitResumed();
                    printf("worker resumed\n");
                }
            }
        }

        float _frequency = 0.0f;
        std::chrono::steady_clock::time_point _time{};

        T _job;
    };
    
}