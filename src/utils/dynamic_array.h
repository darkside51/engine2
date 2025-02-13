#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <variant>
#include <vector>

namespace engine {

template <typename T, size_t Max>
struct dynamic_array {
    std::variant<std::array<T, Max>, std::vector<T>> value;
    uint32_t capacity = 0u;

    dynamic_array(uint32_t size) : capacity(size) {
        if (size <= Max) { [[likely]]
            value = std::array<T, Max>{};
        } else {
            value = std::vector<T>{size};
        } 
    }

    inline const T & operator[](size_t i) const noexcept { 
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]]
            return (*arr)[i];
        } else {
            return std::get<std::vector<T>>(value)[i];
        }
    }

    inline T & operator[](size_t i) noexcept { 
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]]
            return (*arr)[i];
        } else {
            return std::get<std::vector<T>>(value)[i];
        }
    }

    template <size_t Max2>
    inline bool operator == (const dynamic_array<T, Max2> & other) const noexcept {
        if constexpr (Max != Max2) { return false; }

        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]]
            return (*arr == std::get<std::array<T, Max>>(other.value));
        } else {
            return std::get<std::vector<T>>(value) == std::get<std::vector<T>>(other.value);
        }
    }

    inline const T* data() const noexcept {
        if (auto * arr = std::get_if<std::array<T, Max>>(&value)) { [[likely]]
            return arr->data();
        } else {
            return std::get<std::vector<T>>(value).data();
        }
    }

    inline bool empty() const noexcept {
        return capacity == 0u;
    }
};

}