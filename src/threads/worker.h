#pragma once

#include "threads.h"

#include <chrono>
#include <functional>

namespace threads {

    class Worker {
    public:
        ~Worker() {
            stop();
        }

        inline void run() {
            _running.test_and_set(std::memory_order_relaxed);
            _time = std::chrono::steady_clock::now();
            _thread = std::thread(&Worker::loop, this);
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
            _condition.notify_one();
            waitPaused();
            printf("worker paused\n");
        }

        inline void resume() noexcept {
            if (!_running.test_and_set(std::memory_order_relaxed)) {
                _running.notify_one();
                _condition.notify_one();
            }
        }

        inline std::thread::id getThreadId() const noexcept {
            return _threadId;
        }

        inline void emplaceTasks(WorkerTasks && tasks) {
            {
                SpinLock lock(_lockTasks);
                std::move(tasks.begin(), tasks.end(), std::back_inserter(_tasks));
            }
            _condition.notify_one();
        }

        inline void emplaceLoopTasks(WorkerLoopTasks && tasks) {
            {
                SpinLock lock(_lockTasks);
                std::move(tasks.begin(), tasks.end(), std::back_inserter(_loopTasksPatch));
            }
            _condition.notify_one();
        }

    private:
        inline void waitPaused() noexcept {
            if (_paused.test(std::memory_order_relaxed) == false) {
                _paused.wait(false, std::memory_order_relaxed);
                _paused.clear(std::memory_order_relaxed);
            }
        }

        inline void waitResumed() noexcept {
            _running.wait(false, std::memory_order_relaxed);
        }

        [[nodiscard]] inline bool isRunning() const noexcept { return _running.test(std::memory_order_relaxed); }
        [[nodiscard]] inline bool isStopped() const noexcept { return _stop.test(std::memory_order_acquire); }

        void loop() {
            _threadId = std::this_thread::get_id();

            while (!isStopped()) {
                while (isRunning()) {
                    const auto time = std::chrono::steady_clock::now();
                    const std::chrono::duration<float> duration = time - _time; // as default in seconds
			        const auto delta = duration.count();
                    _time = time;

                    {
                        SpinLock lock(_lockTasks);
                        std::move(_loopTasksPatch.begin(), _loopTasksPatch.end(), std::back_inserter(_loopTasks));
                        _loopTasksPatch.clear();
                    }

                    constexpr float infinityTime = 1e+6f; // 1'000'000 seconds
                    _sleepTime = infinityTime;
                    for (auto & loopTask : _loopTasks) {
                        loopTask.elapsed -= delta;
                        if (loopTask.elapsed <= 0.0f) {
                            loopTask.task(loopTask.frequency - loopTask.elapsed);
                            loopTask.elapsed = loopTask.frequency;
                        } 
                        _sleepTime = std::min(_sleepTime, loopTask.elapsed);
                    }

                    WorkerTasks tasks;
                    {
                        SpinLock lock(_lockTasks);
                        tasks = std::move(_tasks);
                        _tasks.clear();
                    }

                    for (auto & task : tasks) {
                        task();
                    }

                    if (auto const t = _sleepTime - delta; t > 0.0f) {
                        std::unique_lock lock(_mutex);
                        _condition.wait_for(lock, std::chrono::duration<float>(t), [this] { 
                            const bool isRun = _running.test(std::memory_order_relaxed);
                            return (isRun && (!_tasks.empty() || !_loopTasksPatch.empty())) || !isRun;
                        });
                    }
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

        std::thread _thread;
        std::thread::id _threadId;
        std::atomic_flag _running;
        std::atomic_flag _paused;
        std::atomic_flag _stop;
        std::condition_variable _condition;
        std::mutex _mutex;

        std::atomic_flag _lockTasks;
        WorkerTasks _tasks;
        WorkerLoopTasks _loopTasksPatch;
        WorkerLoopTasks _loopTasks;

        float _sleepTime = 0.0f;
        std::chrono::steady_clock::time_point _time;
    };
    
}