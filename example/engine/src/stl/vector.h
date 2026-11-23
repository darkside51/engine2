#pragma once

#include "EASTL/vector.h"
// #include <vector>

namespace engine::stl {
    template <typename T, typename Allocator = EASTLAllocatorType>
    using vector = eastl::vector<T, Allocator>;

    inline constexpr struct {
        template <typename... Args>
        constexpr decltype(auto) operator()(Args &&... args) const {
            return eastl::erase(std::forward<Args>(args)...);
        }
    } erase;

    // inline constexpr struct {
    //     template <typename... Args>
    //     constexpr decltype(auto) operator()(Args &&... args) const {
    //         return eastl::erase_if(std::forward<Args>(args)...);
    //     }
    // } erase_if;

    // template <typename T>
    // using vector = std::vector<T>;
}