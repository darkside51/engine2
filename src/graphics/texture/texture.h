#pragma once

#include "graphics/render/rhi/common/runtime.h"

namespace engine::graphics {

struct Texture {
    rhi::runtime::ImagePtr image = nullptr;
    rhi::SamplerInfo sampler = {};

    const rhi::ImageInfo*  getInfo() const noexcept {
        if (image) {
            return &image->getInfo();
        }
        return nullptr;
    }
};

}