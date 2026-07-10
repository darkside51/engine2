#pragma once

#include <cstdint>

namespace map {
    
    struct TileID {
        uint32_t x = 0u;
        uint32_t y = 0u;
        uint8_t z = 0u;
    };

    using EncodedTileID = uint64_t;

    inline EncodedTileID encode(const TileID & tid) noexcept {
        return static_cast<uint64_t>(tid.z) << 40u | static_cast<uint64_t>(tid.y) << 20u | static_cast<uint64_t>(tid.x);
    }

    inline TileID decode(EncodedTileID tid) noexcept {
        constexpr auto kmask = static_cast<uint32_t>(~(0xff'f0 << 16u));
        return {
            .x = static_cast<uint32_t>(tid >> 0u) & kmask,
            .y = static_cast<uint32_t>(tid >> 20u) & kmask,
            .z = static_cast<uint8_t>(tid >> 40u),
        };
    }

}