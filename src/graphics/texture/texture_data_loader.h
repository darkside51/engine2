#pragma once

#include "graphics/render/rhi/common/types.h"
#include "math/math.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace engine::graphics {

enum class TextureLoaderType : uint8_t {
    eKtx = 0u,
    eSail = 1u,
    eRaw = 2u
};

enum class TextureFlags : uint8_t {
    eNone = 0u,
    eAllowSrgbColorSpace = 1u,
    eSupportLinearTiling = 2u,
    eUseMipMaps = 4u,
};
inline TextureFlags operator | (TextureFlags a, TextureFlags b) {
    return static_cast<TextureFlags>(std::to_underlying(a) | std::to_underlying(b));
}
inline TextureFlags operator & (TextureFlags a, TextureFlags b) {
    return static_cast<TextureFlags>(std::to_underlying(a) & std::to_underlying(b));
}

struct TextureMetaInfo {
    vec3u size;
    uint16_t bytesPerRow;
    rhi::Format format;
    uint8_t bitsPerPixel;
    uint8_t numLevels;
    bool hasPalette;
    TextureLoaderType parserType; 
    TextureFlags flags;
};

struct PaletteData {
    std::vector<std::byte> data;
    rhi::Format format;
};

struct TextureCreateInfo {
    TextureMetaInfo metaInfo;
    std::vector<std::vector<std::byte>> data;
};

struct TextureLoadInfo {
    TextureFlags flags = TextureFlags::eNone;
};

TextureCreateInfo parseTextureData(std::span<const std::byte> bytes, const TextureLoadInfo & info = {});
TextureMetaInfo parseTextureMetaInfoOnly(std::span<const std::byte> bytes, const TextureLoadInfo & info = {});
bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info);
}