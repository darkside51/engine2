#pragma once

#include "stl/vector.h"

#include <cstddef>
#include <cstdint>

namespace engine::graphics {

enum class BC_Compression : uint8_t {
    eBC1 = 0u,
    eBC3 = 1u,
    eBC4 = 2u,
    eBC5 = 3u,
    eBC6 = 4u,
    eBC7 = 5u,
};

struct ColorRGBA { std::byte r, g, b, a; };

// this function extracts a 4x4 pixel block from a flat RGBA array.
// it automatically fills pixels outside the texture boundaries using the clamp method (edge ​​repetition).
void extractBlock4x4(const std::byte * srcRgba, uint32_t width, uint32_t height, 
                     uint32_t blockX, uint32_t blockY, ColorRGBA dstBlock[16u]);

stl::vector<std::byte> compressToBC(const std::byte * srcRgba, uint32_t width, uint32_t height, BC_Compression compression);


}