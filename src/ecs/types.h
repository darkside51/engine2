#pragma once

#include "common/type_id.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace engine::ecs {

template <typename T>
inline constexpr bool kTriviallyRelocatable =
#if defined(__cpp_lib_trivially_relocatable)
    std::is_trivially_relocatable_v<T>;
#else
    std::is_trivially_copyable_v<T>;
#endif

constexpr uint16_t kMaxComponents = 256u;
using TypeId = uint32_t;

struct ComponentType;

template <typename T>
inline TypeId typeId() noexcept {
    static const TypeId id = []() noexcept {
        const TypeId v = static_cast<TypeId>(
            rtti::UniqueTypeId<ComponentType, TypeId>::template typeId<std::remove_cvref_t<T>>());
        if (v >= kMaxComponents) {
            assert(false && "too many component types");
            std::abort();
        }
        return v;
    }();
    return id;
}

struct Entity {
    uint32_t index = 0xff'ff'ff'ffu;
    uint32_t generation = 0u;

    bool operator== (const Entity &) const = default;
};

}