#pragma once

#include "threads.h"
#include "lock_free_queue.h"

#include <array>
#include <cassert>
#include <chrono>
#include <functional>

namespace threads {
    
    using ExecutionTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

    class ExecutionContext {
        static constexpr uint8_t kTaskFamiliesCount = 4u;
    public:
        ~ExecutionContext() {
            stop();
        }

        inline void stop() noexcept {
            if (!_stop.test_and_set(std::memory_order_release)) {
                isRunning() ? pause() : resume();
                if (_thread.joinable()) {
                    _thread.join();
                }
                printf("worker stopped\n");
                gThreadsCount.fetch_sub(1u, std::memory_order_release);
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

        // inline void emplaceTasks(WorkerTasks && tasks, uint8_t id = 0u) {
        //     SpinLock lock(_tasks[id].lockTasks);
        //     std::move(tasks.begin(), tasks.end(), std::back_inserter(_tasks[id].tasks));
        // }

        inline void emplaceTask(WorkerTask && task, uint8_t id = 0u) {
            // SpinLock lock(_tasks[id].lockTasks);
            // _tasks[id].tasks.emplace_back(std::move(task));
            _tasks[id].enqueue(std::move(task));
        }

        void performTasks(uint8_t id = 0u) {
            assert(std::this_thread::get_id() == _threadId);

            WorkerTask task = nullptr;
            while (_tasks[id].dequeue(task)) {
                task();
            }
            // WorkerTasks tasks;
            // {
            //     SpinLock lock(_tasks[id].lockTasks);
            //     tasks = std::move(_tasks[id].tasks);
            //     _tasks[id].tasks.clear();
            // }

            // for (auto & task : tasks) {
            //     task();
            // }
        }

        void identifyThreadId() noexcept {
            _threadId = std::this_thread::get_id();
        }

    protected:
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

        std::thread _thread;
        std::thread::id _threadId;
        std::atomic_flag _running;
        std::atomic_flag _paused;
        std::atomic_flag _stop;

        // struct Tasks {
        //     std::atomic_flag lockTasks = ATOMIC_FLAG_INIT;
        //     WorkerTasks tasks;
        // };

        inline static constexpr uint32_t kTasksCapacity = 128u; 
        using Tasks = BoundedQueue<WorkerTask, kTasksCapacity>;
        std::array<Tasks, kTaskFamiliesCount> _tasks;
    };

    template <typename T>
    class Executor : public ExecutionContext {
    public:
        Executor(T && job, float desiredFrameTime, bool immidiateRun = false) : 
            _job(std::move(job)), 
            _desiredFrameTime(desiredFrameTime) 
        {
            if(immidiateRun) {
                run();
            }
        }

        inline void run() {
            _running.test_and_set(std::memory_order_relaxed);
            _time = std::chrono::steady_clock::now();
            _thread = std::thread(&Executor::loop, this);
            gThreadsCount.fetch_add(1u, std::memory_order_release);
        }

    private:
        void loop() {
            identifyThreadId();

            while (!isStopped()) {
                while (isRunning()) {
                    const auto time = std::chrono::steady_clock::now();
                    const std::chrono::duration<float> duration = time - _time; // as default in seconds
				    const auto delta = duration.count();

                    const float t = _desiredFrameTime - delta;

                    if (t > 0.0f) {
                        std::this_thread::sleep_for(std::chrono::duration<float>(t));
                        continue;
                    }

                    _time = std::chrono::steady_clock::now();

                    _job(*this, _time, delta);
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

        float _desiredFrameTime = 0.0f;
        std::chrono::steady_clock::time_point _time{};

        T _job;
    };
    
}