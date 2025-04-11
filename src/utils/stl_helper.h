#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace stl {
    template<class T, std::size_t N>
    auto make_vector(std::array<T, N> && a)-> std::vector<T> {
    return {std::make_move_iterator(std::begin(a)),
            std::make_move_iterator(std::end(a))};
    }

    template<class... T>
    auto make_vector(T && ... t) {
        return make_vector(std::to_array({std::forward<T>(t)...}));
    }

    // steal data from container T, this data must be freed with delete[]
    template <typename T>
    inline std::pair<typename T::value_type*, size_t> steal_data(T& data) {
        const size_t size = data.size();
        auto *buffer = new typename T::value_type[size];
        std::move(data.begin(), data.end(), buffer);
        return  { buffer, size };
    };
}