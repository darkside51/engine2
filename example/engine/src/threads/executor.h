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

    protected:
        inline static constexpr uint8_t kRunning = 0u;
        inline static constexpr uint8_t kPaused  = 1u;
        inline static constexpr uint8_t kStopped = 2u;

    public:
        virtual ~ExecutionContext() { }

        void stop() noexcept {
            // switch to Stopped state regardless of the previous state
            uint8_t oldState = _state.exchange(kStopped, std::memory_order_acq_rel);
            if (oldState == kStopped) {
                return; // already stopped
            }

            // wake up the thread if it was sleeping on pause
            _isCompletePaused.store(false, std::memory_order_release);
            _state.notify_one();

            if (_thread.joinable()) {
                _thread.join();
            }

            printf("worker stopped\n");
            gThreadsCount.fetch_sub(1u, std::memory_order_release);
        }

        void pause() noexcept {
            uint8_t expected = kRunning;
            // change to Paused only if the thread was currently running
            if (_state.compare_exchange_strong(expected, kPaused, std::memory_order_acq_rel)) {
                _isCompletePaused.wait(false, std::memory_order_acquire); // wait pause complete set
                printf("worker paused\n");
            }
        }

        void resume() noexcept {
            uint8_t expected = kPaused;
            // return to Running only if the thread was paused
            if (_state.compare_exchange_strong(expected, kRunning, std::memory_order_acq_rel)) {
                _isCompletePaused.store(false, std::memory_order_release);
                _state.notify_one(); // wake up the thread
            }
        }

        std::thread::id getThreadId() const noexcept {
            return _threadId;
        }

        void emplaceTask(WorkerTask&& task, uint8_t id = 0u) {
            _tasks[id].enqueue(std::move(task));
        }

        void performTasks(uint8_t id = 0u) {
            assert(std::this_thread::get_id() == getThreadId());
            WorkerTask task;
            while (_tasks[id].dequeue(task)) {
                task();
            }
        }

        void performOneTask(uint8_t id = 0u) {
            assert(std::this_thread::get_id() == getThreadId());
            WorkerTask task;
            if (_tasks[id].dequeue(task)) {
                task();
            }
        }

        void identifyThreadId() noexcept {
            _threadId = std::this_thread::get_id();
        }

    protected:
        std::thread _thread;
        std::thread::id _threadId{};

        std::atomic<uint8_t> _state{ kRunning };
        std::atomic<bool> _isCompletePaused{ false }; 

        static constexpr uint32_t kTasksCapacity = 128u; 
        using Tasks = BoundedQueue<WorkerTask, kTasksCapacity>;
        std::array<Tasks, kTaskFamiliesCount> _tasks;
    };

    template <typename T>
    class Executor final : public ExecutionContext {
    public:
        Executor(T&& job, float desiredFrameTime, bool immediateRun = false) 
            : _job(std::move(job))
            , _desiredFrameDuration(desiredFrameTime) {
            if (immediateRun) {
                run();
            }
        }

        ~Executor() override {
            stop(); // call stop before destroying the _job functor
        }

        void run() {
            if (_thread.joinable()) {
                return;
            }

            // if Executor restarts, reset state to Running
            _state.store(kRunning, std::memory_order_release);
            gThreadsCount.fetch_add(1u, std::memory_order_release);
            
            _thread = std::thread(&Executor::loop, this);
        }

    private:
        void loop() {
            identifyThreadId();

            auto nextFrameTime = std::chrono::steady_clock::now();
            auto lastFrameTime = nextFrameTime;

            while (true) {
                uint8_t currentState = _state.load(std::memory_order_acquire);

                if (currentState == kPaused) {
                    _isCompletePaused.store(true, std::memory_order_release);
                    _isCompletePaused.notify_one(); 

                    // optimal pause waiting loop on atomic (C++20 futex)
                    while (currentState == kPaused) {
                        _state.wait(kPaused, std::memory_order_acquire);
                        currentState = _state.load(std::memory_order_acquire);
                    }
                }
                

                // if stop is requested, exit immediately
                if (currentState == kStopped) {
                    return;
                }

                const auto startTime = std::chrono::steady_clock::now();
                const std::chrono::duration<float> delta = startTime - lastFrameTime;
                lastFrameTime = startTime;

                _job(*this, startTime, delta.count());

                // perfect fixed-timestep calculation without burning CPU in a continue-loop
                nextFrameTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(_desiredFrameDuration);
                
                auto now = std::chrono::steady_clock::now();
                if (nextFrameTime < now) {
                    nextFrameTime = now; // protect against "spiral of death" during heavy lags
                }

                // sleep exactly as much as needed until the next frame
                std::this_thread::sleep_until(nextFrameTime);
            }
        }
        
        std::chrono::duration<float> _desiredFrameDuration;
        T _job;
    };
}