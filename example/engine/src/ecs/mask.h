#pragma once

#include "common/hash.h"
#include "stl/array.h"
#include "types.h"

#include <cassert>
#include <cstdint>

namespace engine::ecs {

struct ComponentMask {
    static constexpr size_t kWords = (kMaxComponents + 63u) / 64u;
    using type = stl::array<uint64_t, kWords>;
    type bits{};

    // bits[id >> 6] - array index, (id & 63u) - bit index
    void set(TypeId id) noexcept { 
        assert(id < kMaxComponents);
        bits[id >> 6] |=  (1ull << (id & 63u));
    }
    void reset(TypeId id) noexcept {
        assert(id < kMaxComponents);
        bits[id >> 6] &= ~(1ull << (id & 63u)); 
    }
    bool check(TypeId id) const noexcept {
        assert(id < kMaxComponents);
        return (bits[id >> 6] & (1ull << (id & 63u))) != 0u; 
    }

    // A contain all B bits
    bool contains(const ComponentMask & other) const noexcept {
        for (uint16_t i = 0u; i < bits.size(); ++i) {
            if ((bits[i] & other.bits[i]) != other.bits[i]) {
                return false;
            }
        }
        return true;
    }

    bool empty() const noexcept { 
        for (uint64_t v : bits) {
            if (v != 0u) {
                return false;
            }
        }
        return true;
     }
    bool operator== (const ComponentMask &) const = default;
};

}

namespace engine::hash {

template<>
struct Hasher<ecs::ComponentMask> {
    size_t operator()(const ecs::ComponentMask & m) const noexcept {
        size_t r = 0u;
        for (uint64_t b : m.bits) { 
            hash_combine(r, b);
        }
		
        return r;
    }
};

using ComponentMask_hash = Hasher<ecs::ComponentMask>;

}