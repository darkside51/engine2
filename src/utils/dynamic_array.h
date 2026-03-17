#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

namespace engine {

template <typename T, size_t Max>
struct dynamic_array {
    std::variant<std::array<T, Max>, std::vector<T>> value;
    uint32_t capacity = 0u;

    dynamic_array() = default;

    dynamic_array(std::initializer_list<T> list) : capacity(list.size()) {
        if (capacity <= Max) { [[likely]];
            // value = std::array<T, Max>{};
            std::copy(list.begin(), list.end(), std::get<std::array<T, Max>>(value).data());
        } else {
            value = std::vector<T>(list);
        } 
    }

    explicit dynamic_array(uint32_t size) : capacity(size) {
        if (capacity <= Max) { [[likely]];
            // value = std::array<T, Max>{};
        } else {
            value = std::vector<T>(capacity);
        } 
    }

    explicit dynamic_array(std::span<const T> s) : capacity(s.size()) {
        if (capacity <= Max) { [[likely]];
    //         value = std::array<T, Max>{};
        } else {
            value = std::vector<T>(capacity);
        }

        uint32_t i = 0u;
        for (auto & obj : view()) {
            obj = s[i++];
        }
    }

    void reserve(uint32_t size) {
        if (capacity != 0u) { return; }
        if (size <= Max) { [[likely]];
            //value = std::array<T, Max>{}; // already array as default
        } else {
            value = std::vector<T>(size);
        }
        capacity = size;
    }

    template <size_t Max2>
    inline bool operator == (const dynamic_array<T, Max2> & other) const noexcept {
        if constexpr (Max != Max2) { return false; }

        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
            return (*arr == std::get<std::array<T, Max>>(other.value));
        } else {
            return std::get<std::vector<T>>(value) == std::get<std::vector<T>>(other.value);
        }
    }

    inline const T* data() const noexcept {
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
            return arr->data();
        } else {
            return std::get<std::vector<T>>(value).data();
        }
    }

    std::span<T> view() noexcept {
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
            return {arr->begin(), arr->begin() + capacity};
        } else {
            return std::get<std::vector<T>>(value);
        }
    }

    std::span<const T> view() const noexcept {
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
            return {arr->begin(), arr->begin() + capacity};
        } else {
            return std::get<std::vector<T>>(value);
        }
    }

    template <typename U>
    T& emplace_back(U && v) {
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
            if (capacity < Max - 1) {
                auto & result = (*arr)[capacity++];
                if constexpr(std::is_convertible_v<T, U>) {
                    result = std::forward<U>(v);
                } else {
                    new(&result) T(std::forward<U>(v));
                }
                return result;
            } else {
                assert(false); // todo!
            }
        } else {
            ++capacity;
            return std::get<std::vector<T>>(value).emplace_back(v);
        }
    }

    template <typename ...Args>
    T& emplace_back(Args && ... args) {
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
            if (capacity < Max - 1) {
                auto & result = (*arr)[capacity++];
                new(&result) T{std::forward<Args>(args)...};
                return result;
            } else {
                assert(false); // todo!
            }
        } else {
            ++capacity;
            return std::get<std::vector<T>>(value).emplace_back(std::forward<Args>(args)...);
        }
    }

    inline uint32_t size() const noexcept {
        return capacity;
    } 

    inline bool empty() const noexcept {
        return capacity == 0u;
    }

    inline void clear() noexcept {
        capacity = 0u;
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]];
        } else {
            std::get<std::vector<T>>(value).clear();
        }
    }
};

}