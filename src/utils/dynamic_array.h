#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <variant>
#include <vector>

namespace engine {

template <typename T, size_t Max>
struct dynamic_array {
    std::variant<std::array<T, Max>, std::vector<T>> data;
    uint32_t capacity = 0u;

    dynamic_array(uint32_t size) : capacity(size) {
        if (size <= Max) { [[likely]]
            data = std::array<T, Max>{};
        } else {
            data = std::vector<T>{size};
        } 
    }

    inline const T & operator[](size_t i) const noexcept { 
        if (auto * arr = std::get_if<std::array<T, Max>>(&data)) { [[likely]]
            return (*arr)[i];
        } else {
            return std::get<std::vector<T>>(data)[i];
        }
    }

    template <size_t Max2>
    inline bool operator == (const dynamic_array<T, Max2> & other) const noexcept {
        if constexpr (Max != Max2) { return false; }

        if (auto * arr = std::get_if<std::array<T, Max>>(&data)) { [[likely]]
            return (*arr == std::get<std::array<T, Max>>(other.data));
        } else {
            return std::get<std::vector<T>>(data) == std::get<std::vector<T>>(other.data);
        }
    }
};

}