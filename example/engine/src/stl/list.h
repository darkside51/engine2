#pragma once

#include "EASTL/list.h"
// #include <list>

namespace engine::stl {
    template <typename T, typename Allocator = EASTLAllocatorType>
    using list = eastl::list<T, Allocator>;
}