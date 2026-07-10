#pragma once

#include "EASTL/unordered_map.h"
// #include <unordered_map>

namespace engine::stl {
    template <
                typename Key,
				typename T,
				typename Hash = eastl::hash<Key>,
				typename Predicate = eastl::equal_to<Key>,
				typename Allocator = EASTLAllocatorType,
				bool bCacheHashCode = false
            >
    using unordered_map = eastl::unordered_map<Key, T, Hash, Predicate, Allocator, bCacheHashCode>;

    // template <typename T>
    // using unordered_map = std::unordered_map<T>;
}