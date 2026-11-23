#pragma once

#include "EASTL/span.h"
// #include <span>

namespace engine::stl {
    template <typename T, size_t Extent = eastl::dynamic_extent>
    using span = eastl::span<T, Extent>;

    // template <typename T, size_t Extent = std::dynamic_extent>
    // using span = stl::span<T, Extent>;
}