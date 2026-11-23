#pragma once

#include "types.h"
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace engine::ecs {

struct ComponentMeta {
    TypeId type = 0u;
    uint32_t size  = 0u;
    uint32_t align = 1u;
    bool trivially_relocatable = false;
    bool trivially_destructible = false;
    bool trivially_copyable = false;

    void (*move_construct)(void* dst, void* src) noexcept = nullptr;
    void (*move_replace)(void* dst, void* src) noexcept = nullptr;
    void (*destroy)(void* ptr) noexcept = nullptr;

    template <typename T>
    static ComponentMeta of() noexcept {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
        static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
        return {
            .type = static_cast<uint32_t>(typeId<T>()),
            .size = static_cast<uint32_t>(sizeof(T)), 
            .align = static_cast<uint32_t>(alignof(T)),
            .trivially_relocatable = kTriviallyRelocatable<T>,
            .trivially_destructible = std::is_trivially_destructible_v<T>,
            .trivially_copyable = std::is_trivially_copyable_v<T>,
            .move_construct = [](void* d, void* s) noexcept {
                ::new (d) T(std::move(*static_cast<T*>(s)));
            },
            .move_replace = [](void* d, void* s) noexcept {
                static_cast<T*>(d)->~T();
                ::new (d) T(std::move(*static_cast<T*>(s)));
            },
            .destroy = [](void* p) noexcept { static_cast<T*>(p)->~T(); },
        };
    }
};

}