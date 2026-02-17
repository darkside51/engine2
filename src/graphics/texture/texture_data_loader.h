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

struct TextureMetaInfo {
    vec2u size;
    uint16_t bytesPerLine;
    rhi::Format format;
    uint8_t bpp; // bits per pixels;
    bool hasPalette;
    TextureLoaderType parserType; 
};

struct PaletteData {
    std::vector<std::byte> data;
    rhi::Format format;
};

struct TextureCreateInfo {
    TextureMetaInfo metaInfo;
    std::vector<std::byte> data;
};

enum class LoadFlags : uint8_t {
    eNone = 0u,
    eAllowSrgbColorSpace = 1u,
};
inline LoadFlags operator | (LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(std::to_underlying(a) | std::to_underlying(b));
}
inline LoadFlags operator & (LoadFlags a, LoadFlags b) {
    return static_cast<LoadFlags>(std::to_underlying(a) & std::to_underlying(b));
}

struct TextureLoadInfo {
    LoadFlags flags = LoadFlags::eNone;
};

TextureCreateInfo parseTextureData(std::span<const std::byte> bytes, const TextureLoadInfo & info = {});
TextureMetaInfo parseTextureMetaInfoOnly(std::span<const std::byte> bytes, const TextureLoadInfo & info = {});
bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info);
}