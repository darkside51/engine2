#pragma once

#include "EASTL/array.h"

namespace engine::stl {
    template <typename T, size_t N>
    using array = eastl::array<T, N>;
}