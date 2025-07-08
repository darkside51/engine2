#pragma once

#include "graphics/render/rhi/common/types.h"
#include "math/math.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace engine::graphics {

struct TextureMetaInfo {
    vec2u size;
    uint16_t bytesPerLine;
    rhi::Format format;
    uint8_t bpp; // bits per pixels;
    bool hasPalette;
};

struct TextureCreateInfo {
    TextureMetaInfo metaInfo;
    std::vector<std::byte> data;
};

TextureMetaInfo parseTextureMetaInfoOnly(std::span<const std::byte> bytes);
TextureCreateInfo parseTextureData(std::span<const std::byte> bytes);
bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info);

}