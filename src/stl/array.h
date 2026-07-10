#pragma once

#include "EASTL/array.h"
// #include <array>

namespace engine::stl {
    template <typename T, size_t N>
    using array = eastl::array<T, N>;

    // template <typename T, size_t N>
    // using array = stl::array<T, N>;
}