#pragma once

#include "EASTL/unordered_set.h"
// #include <unordered_set>

namespace engine::stl {
    template <
                typename Value,
				typename Hash = eastl::hash<Value>,
				typename Predicate = eastl::equal_to<Value>,
				typename Allocator = EASTLAllocatorType,
				bool bCacheHashCode = false
            >
    using unordered_set = eastl::unordered_set<Value, Hash, Predicate, Allocator, bCacheHashCode>;

    // template <typename T>
    // using unordered_set = std::unordered_set<T>;
}