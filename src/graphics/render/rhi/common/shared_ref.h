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
        // using Deleter = void(*)(void * ptr);
        using Deleter = std::function<void(void * ptr)>;
        using counter_type = std::atomic<uint32_t>;

        struct ControlBlock {
            counter_type counter = {};
            uint16_t align = 0U;
            uint16_t deleterOffset = 0U;
        };

        struct DefaultValueComposition {
            ControlBlock ctrlBlock = {};
            T data;

            inline static consteval size_t data_offset() noexcept {
                return offsetof(DefaultValueComposition, data);
            }
        };

        struct DefaultValueCompositionWithDeleter {
            ControlBlock ctrlBlock = {};
            T data;
            Deleter deleter = nullptr;

            inline static consteval size_t data_offset() noexcept {
                return offsetof(DefaultValueCompositionWithDeleter, data);
            }

            inline static consteval size_t deleter_offset() noexcept {
                return offsetof(DefaultValueCompositionWithDeleter, deleter);
            }
        };

        sh_ptr(std::nullptr_t) : _ctrlBlock(nullptr) { }
        sh_ptr() : sh_ptr(nullptr) {}

        sh_ptr(void * mem, uint32_t align, uint32_t dOffset, Deleter && d = nullptr) : 
            _ctrlBlock(static_cast<ControlBlock*>(mem)) {
            _ctrlBlock->align = align;
            _ctrlBlock->deleterOffset = d ? dOffset : 0u;
            counter().store(1u, std::memory_order_release);
            if (d) {
                auto * deleterMem = reinterpret_cast<Deleter*>(
                    reinterpret_cast<std::byte*>(get()) + align_value(_ctrlBlock->deleterOffset, alignof(Deleter)));
                new(deleterMem) Deleter{std::move(d)};
            }
        }

        template <typename...Args>
        static sh_ptr make(Args&&...args) {
            auto * mem = malloc(sizeof(DefaultValueComposition));
            auto * objectMem = reinterpret_cast<std::byte*>(mem) + align_value(sizeof(ControlBlock), alignof(T));
            // auto * objectMem = reinterpret_cast<std::byte*>(mem) + DefaultValueComposition::data_offset();
            T* object = new(objectMem) T(std::forward<Args>(args)...);
            return sh_ptr(mem, alignof(T), 0u);
        }

        ~sh_ptr() {
            decrease_counter();
        }

        template <typename U>
        sh_ptr(const sh_ptr<U> & p) : _ctrlBlock(static_cast<ControlBlock*>(p.memory())) {
			_inc();
		}

        sh_ptr(const sh_ptr & p) : _ctrlBlock(p._ctrlBlock) {
			_inc();
		}

		sh_ptr(sh_ptr && p) noexcept : _ctrlBlock(p._ctrlBlock) {
			p._ctrlBlock = nullptr;
		}

        inline void reset() noexcept {
            if (_ctrlBlock) {
                decrease_counter();
                _ctrlBlock = nullptr;
            }
        }

        inline sh_ptr & operator= (std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        inline sh_ptr & operator= (const sh_ptr & p) noexcept {
            if (&p != this && p._ctrlBlock != _ctrlBlock) {
                decrease_counter();
                _ctrlBlock = p._ctrlBlock;
                increase_counter();
            }
            return *this;
        }

        inline sh_ptr & operator= (sh_ptr && p) noexcept {
            if (p._ctrlBlock != _ctrlBlock) {
                decrease_counter();
            }

            _ctrlBlock = p._ctrlBlock;
            p._ctrlBlock = nullptr;
            return *this;
        }

        T * get() noexcept { return reinterpret_cast<T*>(
            reinterpret_cast<std::byte*>(_ctrlBlock) + align_value(sizeof(ControlBlock), _ctrlBlock->align));
        }

        T * get() const noexcept { return reinterpret_cast<T*>(
            reinterpret_cast<std::byte*>(_ctrlBlock) + align_value(sizeof(ControlBlock), _ctrlBlock->align));
        }

        T * operator->() noexcept { return get(); }
        const T * operator->() const noexcept { return get(); }

        T & operator*() noexcept { return *get(); }
        const T & operator*() const noexcept { return *get(); }

        inline bool operator== (std::nullptr_t) const noexcept {
            return _ctrlBlock == nullptr;
        }

        explicit operator bool() const noexcept { return _ctrlBlock != nullptr; }

        void * memory() const noexcept {
            return _ctrlBlock;
        }

    private:
        inline void increase_counter() noexcept {
            if (_ctrlBlock) {
                _inc();
            }
        }

        inline void decrease_counter() noexcept {
            if (_ctrlBlock && _dec() == 0u) {
                if (_ctrlBlock->deleterOffset > 0u) {
                    deleter()(this);
                } else {
                    get()->~T();
                    free(_ctrlBlock);
                }
            }
        }

        inline counter_type _dec() noexcept { return counter().fetch_sub(1u, std::memory_order_release) - 1u; }
		inline counter_type _inc() noexcept { return counter().fetch_add(1u, std::memory_order_release) + 1u; }

        inline counter_type & counter() noexcept {
            return _ctrlBlock->counter;
        }

        inline Deleter & deleter() noexcept {
            return *reinterpret_cast<Deleter*>(
                reinterpret_cast<std::byte*>(get()) + align_value(_ctrlBlock->deleterOffset, alignof(Deleter)));
        }

        ControlBlock * _ctrlBlock = nullptr;
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