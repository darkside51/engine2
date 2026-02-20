#pragma once

#include "../utils/stl_helper.h"

#include <atomic>
#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <stop_token>
		
namespace threads {

    class SpinLock {
	public:
		explicit SpinLock(std::atomic_flag & l) : _lock(l) {
			while (_lock.test_and_set(std::memory_order_release)) {}
		}

		~SpinLock() {
			_lock.clear(std::memory_order_release);
		}

        inline bool try_lock() noexcept {
            return !_lock.test_and_set(std::memory_order_release);
        }

        inline void unlock() noexcept {
            _lock.clear(std::memory_order_release);
        }

	private:
		std::atomic_flag & _lock;
	};

    class RWSpinLock {
    public:
        inline static void readLock(std::atomic<int32_t> & lock) noexcept {
            auto v = lock.load(std::memory_order_acquire);
            while (v < 0 || !lock.compare_exchange_weak(v, v + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                v = lock.load(std::memory_order_acquire);
            }
        }
        inline static void readUnlock(std::atomic<int32_t> & lock) noexcept {
            lock.fetch_sub(1, std::memory_order_release);
        }

        inline static void writeLock(std::atomic<int32_t> & lock) noexcept {
            int32_t v = 0;
            while (!lock.compare_exchange_weak(v, -1, std::memory_order_acq_rel, std::memory_order_relaxed)) { 
                v = 0; // it will set it to what it currently held
            }
        }
        inline static void writeUnlock(std::atomic<int32_t> & lock) noexcept {
            lock.store(0, std::memory_order_release);
        }

        RWSpinLock(std::atomic<int32_t> & rwLock) : _rwLock(rwLock) {}
        ~RWSpinLock() { unlock(); }

        inline void readLock() noexcept {
            readLock(_rwLock);
            _state = State::read;
        }

        inline void writeLock() noexcept {
            writeLock(_rwLock);
            _state = State::write;
        }

        inline void unlock() noexcept {
            switch (_state) {
            case State::read:
                readUnlock(_rwLock);
                break;
            case State::write:
                writeUnlock(_rwLock);
                break;
            default:
                break;
            }

            _state = State::free;
        }

    private:
        enum class State : uint8_t {
            free = 0u, read = 1u, write = 2u
        };
        std::atomic<int32_t> & _rwLock;
        State _state = State::free;
    };

    class ReadLock {
    public:
        ReadLock(std::atomic<int32_t> & rwLock) : _rwLock(rwLock) {
            RWSpinLock::readLock(_rwLock);
        }

        ~ReadLock() {
            RWSpinLock::readUnlock(_rwLock);
        }

    private:
        std::atomic<int32_t> & _rwLock;
    };

    class WriteLock {
    public:
        WriteLock(std::atomic<int32_t> & rwLock) : _rwLock(rwLock) {
            RWSpinLock::writeLock(_rwLock);
        }

        ~WriteLock() {
            RWSpinLock::writeUnlock(_rwLock);
        }

    private:
        std::atomic<int32_t> & _rwLock;
    };

    using StopSource = std::stop_source;
    using StopToken = std::stop_token;

    class StopSource2 {
    public:
        StopSource2() = default;
        StopSource2(StopSource && source) : _source(std::move(source)) {}
        StopSource2(StopSource2 && other) noexcept : _source(std::move(other._source)) {}
        ~StopSource2() {
            _source.request_stop();
        }

        StopSource2 & operator = (StopSource2 && other) noexcept {
            _source = std::move(other._source);
            return *this;
        }

        [[nodiscard]] StopToken get_token() const noexcept {
            return _source.get_token();
        }

        void request_stop() const noexcept {
            _source.request_stop();
        }

        bool stop_requested() const noexcept {
            return _source.stop_requested();
        }

        bool stop_possible() const noexcept {
            return _source.stop_possible();
        }

    private:
        StopSource _source;
    };

    class WaitToken {
    public:
        explicit WaitToken(std::atomic_flag & flag) : _flag (flag) {}

        void request_complete() noexcept {
            _flag.test_and_set(std::memory_order_relaxed);
            // _flag.notify_one();
        }

    private:
        std::atomic_flag & _flag;
    };

    class WaitSource {
    public:
        WaitToken get_token() noexcept {
            return WaitToken{_flag};
        }

        void request_wait() const noexcept {
            while (!_flag.test(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            // _flag.wait(false, std::memory_order_relaxed);
        }

        void request_complete() noexcept {
            _flag.test_and_set(std::memory_order_relaxed);
            // _flag.notify_one();
        }

        void reset() noexcept {
            _flag.clear(std::memory_order_relaxed);
        }

        void request_wait_and_reset() noexcept {
            request_wait();
            reset();
        }

    private:
        std::atomic_flag _flag = ATOMIC_FLAG_INIT;
    };

    ////////////////////
    template<typename R>
    using function = std::move_only_function<R>;

    using WorkerTask = function<void()>;
    using WorkerTasks = std::vector<WorkerTask>;

    struct WorkerLoopTask {
        using Task = function<void(float)>;
        float frequency = 0.0f;
        float elapsed = 0.0f;
        Task task;

        WorkerLoopTask(float freq, function<void(float)> && task) : frequency(freq), elapsed(freq), task(std::move(task)) {}
        WorkerLoopTask(WorkerLoopTask && other) noexcept : frequency(other.frequency), elapsed(other.elapsed), task(std::move(other.task)) {}

        WorkerLoopTask & operator= (WorkerLoopTask && other) noexcept {
            frequency = other.frequency;
            elapsed = other.elapsed;
            task = std::move(other.task);
            return *this;
        }
    };
    using WorkerLoopTasks = std::vector<WorkerLoopTask>;

    template <typename F>
    WorkerTask makeTask(F && f, const StopSource & source) {
        return std::bind(std::move(f), source.get_token());
    }

    template <typename F>
    WorkerLoopTask makeLoopTask(float frequency, F && f, const StopSource & source) {
       return {frequency, std::bind(std::move(f), std::placeholders::_1, source.get_token())};
    }

}