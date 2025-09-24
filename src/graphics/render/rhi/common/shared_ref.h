#pragma once

#include "common/align.h"

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <functional>

namespace engine::rhi::runtime {
    
    template <typename T>
    class sh_ptr {
    public:
        using Deleter = std::function<void(void * ptr)>;
        using counter_type = std::atomic<uint32_t>;

        struct BlockPtr {
            counter_type counter = {};
            uint16_t align = 0U;
            uint16_t sz = 0U;
        };

        struct Value {
            BlockPtr blockPtr = {};
            T data;
            Deleter deleter = nullptr;
        };

        sh_ptr(std::nullptr_t) : _blockPtr(nullptr) { }
        sh_ptr() : sh_ptr(nullptr) {}

        sh_ptr(void * mem, uint32_t align, uint32_t sz, Deleter && d = nullptr) : 
            _blockPtr(static_cast<BlockPtr*>(mem)) {
            _blockPtr->align = align;
            _blockPtr->sz = sz;
            counter().store(1u, std::memory_order_release);
            auto * deleterMem = reinterpret_cast<Deleter*>(reinterpret_cast<std::byte*>(get()) + align_value(_blockPtr->sz, alignof(Deleter)));
            d ? new(deleterMem) Deleter{std::move(d)} : new(deleterMem) Deleter{};
        }

        template <typename...Args>
        static sh_ptr make(Args&&...args) {
            auto * mem = malloc(sizeof(Value));
            auto * objectMem = reinterpret_cast<std::byte*>(mem) + align_value(sizeof(BlockPtr), alignof(T));
            T* object = new(objectMem) T(std::forward<Args>(args)...);
            return sh_ptr(mem, alignof(T), sizeof(T));
        }

        ~sh_ptr() {
            decrease_counter();
        }

        template <typename U>
        sh_ptr(const sh_ptr<U> & p) : _blockPtr(static_cast<BlockPtr*>(p.memory())) {
			_inc();
		}

        sh_ptr(const sh_ptr & p) : _blockPtr(p._blockPtr) {
			_inc();
		}

		sh_ptr(sh_ptr && p) noexcept : _blockPtr(p._blockPtr) {
			p._blockPtr = nullptr;
		}

        inline void reset() noexcept {
            if (_blockPtr) {
                decrease_counter();
                _blockPtr = nullptr;
            }
        }

        inline sh_ptr & operator= (std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        inline sh_ptr & operator= (const sh_ptr & p) noexcept {
            if (&p != this && p._blockPtr != _blockPtr) {
                decrease_counter();
                _blockPtr = p._blockPtr;
                increase_counter();
            }
            return *this;
        }

        inline sh_ptr & operator= (sh_ptr && p) noexcept {
            if (p._blockPtr != _blockPtr) {
                decrease_counter();
            }

            _blockPtr = p._blockPtr;
            p._blockPtr = nullptr;
            return *this;
        }

        T * get() noexcept { return reinterpret_cast<T*>(
            reinterpret_cast<std::byte*>(_blockPtr) + align_value(sizeof(BlockPtr), _blockPtr->align));
        }

        T * get() const noexcept { return reinterpret_cast<T*>(
            reinterpret_cast<std::byte*>(_blockPtr) + align_value(sizeof(BlockPtr), _blockPtr->align));
        }

        T * operator->() noexcept { return get(); }
        const T * operator->() const noexcept { return get(); }

        T & operator*() noexcept { return *get(); }
        const T & operator*() const noexcept { return *get(); }

        explicit operator bool() const noexcept { return _blockPtr != nullptr; }

        void * memory() const noexcept {
            return _blockPtr;
        }

    private:
        inline void increase_counter() noexcept {
            if (_blockPtr) {
                _inc();
            }
        }

        inline void decrease_counter() noexcept {
            if (_blockPtr && _dec() == 0u) {
                if (auto & d = deleter()) {
                    d(this);
                } else {
                    get()->~T();
                    free(_blockPtr);
                }
            }
        }

        inline counter_type _dec() noexcept { return counter().fetch_sub(1u, std::memory_order_release) - 1u; }
		inline counter_type _inc() noexcept { return counter().fetch_add(1u, std::memory_order_release) + 1u; }

        inline counter_type & counter() noexcept {
            return _blockPtr->counter;
        }

        inline Deleter & deleter() noexcept {
            return *reinterpret_cast<Deleter*>(reinterpret_cast<std::byte*>(get()) + align_value(_blockPtr->sz, alignof(Deleter)));
        }

        BlockPtr * _blockPtr = nullptr;
    };

    //////
    template <typename T, typename Deleter>
    class sh_ptr2 {
    public:
        using counter_type = std::atomic<uint32_t>;

        struct Value {
            counter_type counter = {};
            T data;
        };

        sh_ptr2(std::nullptr_t) : _counter(nullptr), _data(nullptr) { }
        sh_ptr2() : sh_ptr2(nullptr) {}

        sh_ptr2(void * mem, uint32_t align) : 
            _counter(static_cast<counter_type*>(mem)),
            _data(reinterpret_cast<T*>(
                reinterpret_cast<std::byte*>(mem) + align_value(sizeof(counter_type), align))) {
            _counter->store(1u, std::memory_order_release);
        }

        template <typename...Args>
        static sh_ptr2 make(Args&&...args) {
            auto * mem = malloc(sizeof(Value));
            auto * objectMem = reinterpret_cast<std::byte*>(mem) + align_value(sizeof(counter_type), alignof(T));
            T* object = new(objectMem) T(std::forward<Args>(args)...);
            return sh_ptr2(mem, alignof(T));
        }

        ~sh_ptr2() {
            decrease_counter();
        }

        template <typename U>
        sh_ptr2(const sh_ptr2<U, Deleter> & p) : _counter(static_cast<counter_type*>(p.memory())), 
            _data(static_cast<T*>(p.get())) {
			_inc();
		}

        sh_ptr2(const sh_ptr2 & p) : _counter(p._counter), _data(p._data) {
			_inc();
		}

		sh_ptr2(sh_ptr2 && p) noexcept : _counter(p._counter), _data(p._data) {
			p._counter = nullptr;
            p._data = nullptr;
		}

        inline void reset() noexcept {
            if (_counter) {
                decrease_counter();
                _counter = nullptr;
                _data = nullptr;
            }
        }

        inline sh_ptr2 & operator= (std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        inline sh_ptr2 & operator= (const sh_ptr2 & p) noexcept {
            if (&p != this && p._counter != _counter) {
                decrease_counter();
                _counter = p._counter;
                _data = p._data;
                increase_counter();
            }
            return *this;
        }

        inline sh_ptr2 & operator= (sh_ptr2 && p) noexcept {
            if (p._counter != _counter) {
                decrease_counter();
            }

            _counter = p._counter;
            _data = p._data;
            p._counter = nullptr;
            p._data = nullptr;
            return *this;
        }

        T * get() noexcept { return _data; }
        T * get() const noexcept { return _data; }

        T * operator->() noexcept { return _data; }
        const T * operator->() const noexcept { return _data; }

        T & operator*() noexcept { return *_data; }
        const T & operator*() const noexcept { return *_data; }

        explicit operator bool() const noexcept { return _data != nullptr; }

        void * memory() const noexcept {
            return _counter;
        }

    private:
        inline void increase_counter() noexcept {
            if (_counter) {
                _inc();
            }
        }

        inline void decrease_counter() noexcept {
            if (_counter && _dec() == 0u) {
                Deleter::_(this);
            }
        }

        inline counter_type _dec() noexcept { return _counter->fetch_sub(1u, std::memory_order_release) - 1u; }
		inline counter_type _inc() noexcept { return _counter->fetch_add(1u, std::memory_order_release) + 1u; }

        counter_type * _counter = nullptr;
        T * _data = nullptr;
    };
}