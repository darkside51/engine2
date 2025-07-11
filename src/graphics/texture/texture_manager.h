#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/runtime.h"

#include <unordered_map>

namespace engine::graphics {

class TextureManager {
    using ImagesMap = std::unordered_map<std::string, rhi::runtime::ImagePtr, hash::String_hash, std::equal_to<>>;
public:
private:
    rhi::runtime::ImagePtr _defaultImage;
    rhi::runtime::ImagePtr _whiteImage;
    rhi::runtime::ImagePtr _blackImage;
};

}