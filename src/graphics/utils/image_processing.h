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

stl::vector<std::byte> compressToBC(
    const std::byte * srcData, uint8_t bytesPepPixel, 
    uint32_t width, uint32_t height, BC_Compression compression
);


}