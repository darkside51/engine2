#pragma once

#include "common/type_id.h"
#include "stl/array.h"

#include <cstdint>

namespace engine::ecs {

constexpr uint16_t kMaxComponents = 256u;
using ComponentMask = stl::array<uint64_t, kMaxComponents / sizeof(uint64_t) / 8u>; // 8u = bit in byte

static inline size_t typeId() noexcept {
    struct Component;
    return run_time_type_id::UniqueTypeId<Component>::typeId();
}

struct Entity {
    uint32_t index = 0xff'ff'ff'ffu;
    uint32_t generation = 0u;
};

}