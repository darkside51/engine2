#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "stl/unordered_map.h"
#include "common/hash.h"

#include <string>
#include <string_view>

namespace engine {

class TextureViewer {
public:
    void setTexture(std::string_view name, const rhi::runtime::ImagePtr & image);
    void removeTexture(std::string_view name);
    void show(bool &showed);
    void destroy();

private:
    stl::unordered_map<std::string, rhi::runtime::ImagePtr, hash::String_hash, std::equal_to<>> _images;
    std::string_view _currentName = "";
    int _currentImageIdx = 0;
    void* _texture = nullptr;
};

}