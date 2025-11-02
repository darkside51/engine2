#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/runtime.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::graphics {

struct Frame2D {
    std::array<float, 2u> uvMin;
    std::array<float, 2u> uvMax;
};

struct ImageFrame2D {
    Frame2D frame;
    rhi::runtime::ImagePtr image = nullptr;
};

class TextureManager {
    using ImagesMap = std::unordered_map<std::string, rhi::runtime::ImagePtr, hash::String_hash, std::equal_to<>>;

public:
    bool addTexture(std::string_view name, rhi::runtime::ImagePtr image);
    bool removeTexture(std::string_view name);
    bool removeTexture(rhi::runtime::ImagePtr image);

    rhi::runtime::ImagePtr findTexture(std::string_view name) noexcept;

private:
    rhi::runtime::ImagePtr _defaultImage;
    rhi::runtime::ImagePtr _whiteImage;
    rhi::runtime::ImagePtr _blackImage;
    rhi::runtime::ImagePtr _transparentImage;

    ImagesMap _images;
};

}