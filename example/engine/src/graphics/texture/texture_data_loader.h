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
    eRaw = 2u,
    eStb = 3u,
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
    stl::vector<std::byte> data;
    rhi::Format format;
};

using TextureLevelData = stl::vector<std::byte>;
using TextureLayerData = stl::vector<TextureLevelData>;

struct TextureCreateInfo {
    TextureMetaInfo metaInfo;
    TextureLayerData data;
};

struct TextureLoadInfo {
    TextureFlags flags = TextureFlags::eNone;
};

TextureCreateInfo parseTextureData(stl::span<const std::byte> bytes, const TextureLoadInfo & info = {});
TextureMetaInfo parseTextureMetaInfoOnly(stl::span<const std::byte> bytes, const TextureLoadInfo & info = {});
bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info);

enum class ImageByteEncoder : uint8_t {
    eJpg = 0u,
    ePng = 1u,
    eWebp = 2u,
    // ...
    eCount
};

stl::vector<std::byte> encodeBytes(stl::span<std::byte> bytes, rhi::Format format, ImageByteEncoder encoder, uint16_t w, uint16_t h);
}