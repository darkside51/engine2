#pragma once

#include "stl/span.h"
#include "stl/vector.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <memory>
#include <algorithm>
#include <new>

namespace engine {

template <typename T, size_t Max>
struct dynamic_array {
    inline static consteval size_t maxArrayCount() noexcept { return Max; }

private:
    alignas(T) std::byte stack_buffer[Max * sizeof(T)];
    stl::vector<T> heap_vector;
    uint32_t current_size = 0u;

    inline bool is_on_stack() const noexcept { return heap_vector.empty(); }

    T * stack_data() noexcept { return reinterpret_cast<T *>(stack_buffer); }
    const T * stack_data() const noexcept { return reinterpret_cast<const T *>(stack_buffer); }

    void destroy_stack_elements() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (uint32_t i = 0u; i < current_size; ++i) {
                stack_data()[i].~T();
            }
        }
    }

public:
    dynamic_array() = default;

    dynamic_array(std::initializer_list<T> list) {
        reserve(list.size());
        for (const auto& item : list) {
            emplace_back(item);
        }
    }

    explicit dynamic_array(uint32_t size) {
        resize(size);
    }

    explicit dynamic_array(stl::span<const T> s) {
        reserve(s.size());
        for (const auto& item : s) {
            emplace_back(item);
        }
    }

    ~dynamic_array() {
        clear();
    }

    dynamic_array(const dynamic_array & other) {
        current_size = other.current_size;
        if (other.is_on_stack()) {
            for (uint32_t i = 0u; i < current_size; ++i) {
                new (stack_data() + i) T(other.stack_data()[i]);
            }
        } else {
            heap_vector = other.heap_vector;
        }
    }

    dynamic_array(dynamic_array && other) noexcept {
        current_size = other.current_size;
        if (other.is_on_stack()) {
            for (uint32_t i = 0u; i < current_size; ++i) {
                new (stack_data() + i) T(std::move(other.stack_data()[i]));
            }
            other.destroy_stack_elements();
            other.current_size = 0u;
        } else {
            heap_vector = std::move(other.heap_vector);
            other.current_size = 0u;
        }
    }

    dynamic_array & operator= (dynamic_array other) noexcept {
        swap(other);
        return *this;
    }

    void swap(dynamic_array & other) noexcept {
        using std::swap;
        if (is_on_stack() && other.is_on_stack()) {
            uint32_t min_size = std::min(current_size, other.current_size);
            for (uint32_t i = 0u; i < min_size; ++i) {
                swap(stack_data()[i], other.stack_data()[i]);
            }
            if (current_size > min_size) {
                for (uint32_t i = min_size; i < current_size; ++i) {
                    new (other.stack_data() + i) T(std::move(stack_data()[i]));
                    stack_data()[i].~T();
                }
            } else if (other.current_size > min_size) {
                for (uint32_t i = min_size; i < other.current_size; ++i) {
                    new (stack_data() + i) T(std::move(other.stack_data()[i]));
                    other.stack_data()[i].~T();
                }
            }
        } else if (!is_on_stack() && !other.is_on_stack()) {
            swap(heap_vector, other.heap_vector);
        } else {
            dynamic_array * heap_side = !is_on_stack() ? this : &other;
            dynamic_array * stack_side = is_on_stack() ? this : &other;

            auto temp_heap = std::move(heap_side->heap_vector);
            
            for (uint32_t i = 0u; i < stack_side->current_size; ++i) {
                new (heap_side->stack_data() + i) T(std::move(stack_side->stack_data()[i]));
            }
            stack_side->destroy_stack_elements();
            
            stack_side->heap_vector = std::move(temp_heap);
        }
        swap(current_size, other.current_size);
    }

    void clear() noexcept {
        if (is_on_stack()) {
            destroy_stack_elements();
        } else {
            heap_vector.clear();
        }
        current_size = 0u;
    }

    void reserve(uint32_t size) {
        if (size <= Max || !is_on_stack()) {
            if (!is_on_stack()) heap_vector.reserve(size);
            return;
        }
        
        heap_vector.reserve(size); // ?
        for (uint32_t i = 0u; i < current_size; ++i) {
            heap_vector.emplace_back(std::move(stack_data()[i]));
        }
        destroy_stack_elements();
    }

    void resize(uint32_t size) {
        if (size <= current_size) {
            if (is_on_stack()) {
                for (uint32_t i = size; i < current_size; ++i) {
                    stack_data()[i].~T();
                }
            } else {
                heap_vector.resize(size);
            }
            current_size = size;
            return;
        }

        reserve(size);
        if (is_on_stack()) {
            for (uint32_t i = current_size; i < size; ++i) {
                new (stack_data() + i) T();
            }
        } else {
            heap_vector.resize(size);
        }
        current_size = size;
    }

    template <typename ...Args>
    T& emplace_back(Args && ... args) {
        if (is_on_stack()) {
            if (current_size < Max) {
                T* target = stack_data() + current_size;
                new(target) T{std::forward<Args>(args)...};
                ++current_size;
                return *target;
            }
            reserve(Max * 2u >= 4u ? Max * 2u : 4u);
        }
        
        heap_vector.emplace_back(std::forward<Args>(args)...);
        current_size = heap_vector.size();
        return heap_vector.back();
    }

    inline uint32_t size() const noexcept { return current_size; }
    inline bool empty() const noexcept { return current_size == 0u; }

    T* data() noexcept { return is_on_stack() ? stack_data() : heap_vector.data(); }
    const T* data() const noexcept { return is_on_stack() ? stack_data() : heap_vector.data(); }

    T& operator[](size_t index) noexcept { return data()[index]; }
    const T& operator[](size_t index) const noexcept { return data()[index]; }
 
    stl::span<T> view() noexcept {
        return stl::span<T>(data(), current_size);
    }

    stl::span<const T> view() const noexcept {
        return stl::span<const T>(data(), current_size);
    }

    template <size_t Max2>
    [[nodiscard]] inline bool operator== (const dynamic_array<T, Max2>& other) const noexcept {
        if (current_size != other.size()) { return false; }
        return view() == other.view();
    }
};

} // namespace engine
