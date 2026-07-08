#pragma once

#include "EASTL/vector.h"
// #include <vector>

namespace engine::stl {
    template <typename T, typename Allocator = EASTLAllocatorType>
    using vector = eastl::vector<T, Allocator>;

    // template <typename T>
    // using vector = std::vector<T>;
}