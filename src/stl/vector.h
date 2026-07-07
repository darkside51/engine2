#pragma once

#include "EASTL/vector.h"

namespace engine::stl {
    template <typename T, typename Allocator = EASTLAllocatorType>
    using vector = eastl::vector<T, Allocator>;
}