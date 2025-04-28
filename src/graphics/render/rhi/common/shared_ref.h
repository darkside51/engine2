#pragma once

#include "common/memory.h"

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <functional>

namespace engine::rhi::runtime {
    
    template <typename T>
    class sh_ptr {
    public:
        using Deleter = std::function<void(void * sh_ptr)>;
        using counter_type = std::atomic<uint32_t>;

        sh_ptr(std::nullptr_t) : _counter(nullptr), _data(nullptr), _deleter(nullptr) { }

        explicit sh_ptr(void * mem, uint32_t align, Deleter && d = nullptr) : 
            _counter(static_cast<counter_type*>(mem)),
            _data(reinterpret_cast<T*>(
                reinterpret_cast<std::byte*>(mem) + align_value(sizeof(counter_type), align))),
            _deleter(std::move(d)) {
            _counter->store(1u, std::memory_order_release);
        }

        ~sh_ptr() {
            decrease_counter();
        }

        template <typename U>
        sh_ptr(sh_ptr<U> & p) : _counter(p.getCounter()), _data(static_cast<T*>(p.get())), _deleter(p.getDeleter()) {
			_inc();
		}

        sh_ptr(const sh_ptr & p) : _counter(p._counter), _data(p._data), _deleter(p._deleter) {
			_inc();
		}

		sh_ptr(sh_ptr && p) noexcept : _counter(p._counter), _data(p._data), _deleter(std::move(p._deleter)) {
			p._counter = nullptr;
            p._data = nullptr;
            p._deleter = nullptr;
		}

        inline void reset() noexcept {
            if (_counter) {
                decrease_counter();
                _counter = nullptr;
                _data = nullptr;
                _deleter = nullptr;
            }
        }

        inline sh_ptr & operator= (std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        inline sh_ptr & operator= (const sh_ptr & p) noexcept {
            if (&p != this && p._counter != _counter) {
                decrease_counter();
                _counter = p._counter;
                _data = p._data;
                _deleter = p._deleter;
                increase_counter();
            }
            return *this;
        }

        inline sh_ptr & operator= (sh_ptr && p) noexcept {
            if (p._counter != _counter) {
                decrease_counter();
            }

            _counter = p._counter;
            _data = p._data;
            _deleter = std::move(p._deleter);
            p._counter = nullptr;
            p._data = nullptr;
            p._deleter = nullptr;
            return *this;
        }

        counter_type * getCounter() const noexcept { return _counter; }

        T * get() noexcept { return _data; }
        const T * get() const noexcept { return _data; }

        T * operator->() noexcept { return _data; }
        const T * operator->() const noexcept { return _data; }

        T & operator*() noexcept { return *_data; }
        const T & operator*() const noexcept { return *_data; }

        void * memory() noexcept {
            return _counter;
        }

        void setDeleter(Deleter && d) noexcept { _deleter = std::move(d); }
        const Deleter & getDeleter() const noexcept { return _deleter; }

    private:
        inline void increase_counter() noexcept {
            if (_counter) {
                _inc();
            }
        }

        inline void decrease_counter() noexcept {
            if (_counter && _dec() == 0u) {
                if (_deleter) {
                    _deleter(this);
                } else {
                    _data->~T();
                    delete _counter;
                }
            }
        }

        inline counter_type _dec() noexcept { return _counter->fetch_sub(1u, std::memory_order_release) - 1u; }
		inline counter_type _inc() noexcept { return _counter->fetch_add(1u, std::memory_order_release) + 1u; }

        counter_type * _counter = nullptr;
        T * _data = nullptr;
        Deleter _deleter = nullptr;
    };

}