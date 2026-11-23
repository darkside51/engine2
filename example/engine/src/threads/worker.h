#pragma once

#include "threads.h"
#include "stl/helper.h"

#include <chrono>
#include <functional>

namespace threads {
    
    class Worker {
    protected:
        // encode states with clear flags
        static constexpr uint8_t kRunning = 0u;
        static constexpr uint8_t kPaused  = 1u;
        static constexpr uint8_t kStopped = 2u;

    public:
        Worker() = default;

        ~Worker() {
            // safely call stop only if we are not inside the worker thread itself
            if (_thread.joinable() && _thread.get_id() != std::this_thread::get_id()) {
                stop();
            }
        }

        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;
        Worker(Worker&&) = delete;
        Worker& operator=(Worker&&) = delete;

        inline void run() {
            if (_thread.joinable()) return;

            _state.store(kRunning, std::memory_order_release);
            _wake.store(false, std::memory_order_release);
            _time = std::chrono::steady_clock::now();

            _thread = std::thread(&Worker::loop, this);
            gThreadsCount.fetch_add(1u, std::memory_order_release);
        }

        inline void stop() noexcept {
            const uint8_t oldState = _state.exchange(kStopped, std::memory_order_acq_rel);
            if (oldState == kStopped) return;

            // wake up from both pause (atomic wait) and execution sleep (wake wait)
            _state.notify_one();
            _wake.store(true, std::memory_order_release);
            _wake.notify_one();

            if (_thread.joinable()) {
                _thread.join();
                gThreadsCount.fetch_sub(1u, std::memory_order_release);
            }
        }

        inline void pause() noexcept {
            uint8_t expected = kRunning;
            if (_state.compare_exchange_strong(expected, kPaused,
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
                _wake.store(true, std::memory_order_release);
                _wake.notify_one();
            }
        }

        inline void resume() noexcept {
            uint8_t expected = kPaused;
            if (_state.compare_exchange_strong(expected, kRunning,
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
                _state.notify_one();
            }
        }

        inline std::thread::id getThreadId() const noexcept {
            return _threadId;
        }

        inline bool isRunning() const noexcept {
            return _state.load(std::memory_order_acquire) == kRunning;
        }

        inline void emplaceTasks(WorkerTasks&& tasks) {
            {
                SpinLock lock(_lockTasks);
                _tasks.insert(_tasks.end(),
                    engine::stl::make_move_iterator(tasks.begin()),
                    engine::stl::make_move_iterator(tasks.end()));
            }
            wakeUp();
        }

        inline void emplaceTask(WorkerTask&& task) {
            {
                SpinLock lock(_lockTasks);
                _tasks.emplace_back(std::move(task));
            }
            wakeUp();
        }

        inline void emplaceLoopTasks(WorkerLoopTasks&& tasks) {
            {
                SpinLock lock(_lockTasks);
                _loopTasksPatch.insert(_loopTasksPatch.end(),
                    engine::stl::make_move_iterator(tasks.begin()),
                    engine::stl::make_move_iterator(tasks.end()));
            }
            wakeUp();
        }

    private:
        inline void wakeUp() noexcept {
            _wake.store(true, std::memory_order_release);
            _wake.notify_one();
        }

        void loop() {
            _threadId = std::this_thread::get_id();

            while (true) {
                uint8_t currentState = _state.load(std::memory_order_acquire);
                if (currentState == kStopped) return;

                // pause via atomic wait without blocking mutexes
                if (currentState == kPaused) {
                    _state.wait(kPaused, std::memory_order_acquire);
                    _time = std::chrono::steady_clock::now();
                    continue;
                }

                // --- hot path ---

                // reset wake flag before checking queues to avoid lost wakeups (race condition)
                _wake.store(false, std::memory_order_release);

                const auto now = std::chrono::steady_clock::now();
                const float delta = std::chrono::duration<float>(now - _time).count();
                _time = now;

                // apply loop tasks patch if any exist
                {
                    SpinLock lock(_lockTasks);
                    if (!_loopTasksPatch.empty()) {
                        _loopTasks.insert(_loopTasks.end(),
                            engine::stl::make_move_iterator(_loopTasksPatch.begin()),
                            engine::stl::make_move_iterator(_loopTasksPatch.end()));
                        _loopTasksPatch.clear();
                    }
                }

                // execute periodic loop tasks
                constexpr float infinityTime = 1e6f;
                float minWait = infinityTime;
                
                for (auto& lt : _loopTasks) {
                    lt.elapsed -= delta;
                    if (lt.elapsed <= 0.0f) {
                        lt.task(lt.frequency - lt.elapsed);
                        lt.elapsed = lt.frequency;
                    }
                    minWait = std::min(minWait, lt.elapsed);
                }

                // extract one-off tasks if any exist
                WorkerTasks tasks;
                {
                    SpinLock lock(_lockTasks);
                    if (!_tasks.empty()) {
                        tasks = std::move(_tasks);
                        _tasks.clear();
                    }
                }
                
                for (auto& task : tasks) {
                    task();
                }

                // check if new tasks arrived during execution to prevent immediate sleep
                if (_wake.load(std::memory_order_acquire)) {
                    continue;
                }

                // hybrid sleep strategy or futex wait if no loop tasks exist
                if (_loopTasks.empty()) {
                    // deep futex sleep if there is absolutely no periodic work
                    uint8_t s = _state.load(std::memory_order_relaxed);
                    if (s == kRunning && !_wake.load(std::memory_order_relaxed)) {
                        _wake.wait(false, std::memory_order_acquire);
                    }
                } else if (minWait > 0.0f) {
                    // frame sleep: 80% duration via OS sleep_for
                    const auto sleepDur = std::chrono::duration<float>(minWait * 0.8f);
                    std::this_thread::sleep_for(sleepDur);

                    // remaining 20% duration via low-latency spin-wait
                    const auto deadline = now + std::chrono::duration<float>(minWait);
                    while (std::chrono::steady_clock::now() < deadline) {
                        if (_state.load(std::memory_order_relaxed) != kRunning) break;
                        if (_wake.load(std::memory_order_relaxed)) break;
                        
                        #if defined(__x86_64__) || defined(__i386__)
                        __builtin_ia32_pause();
                        #elif defined(__aarch64__) || defined(__arm__)
                        __builtin_arm_yield();
                        #endif
                    }
                }
            }
        }

        std::thread _thread;
        std::thread::id _threadId{};

        std::atomic<uint8_t> _state{ kRunning };
        std::atomic<bool>    _wake{ false };

        std::atomic_flag _lockTasks;
        WorkerTasks _tasks;
        WorkerLoopTasks _loopTasksPatch;
        WorkerLoopTasks _loopTasks;

        std::chrono::steady_clock::time_point _time;
    };

} // namespace threads