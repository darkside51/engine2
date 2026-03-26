#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/shared_ref.h"

namespace engine::graphics {

struct ImageHandler {
    rhi::runtime::ImagePtr image = nullptr;
};

using ImageHandlerPtr = rhi::runtime::sh_ptr<ImageHandler>;

// struct Texture {
//     rhi::runtime::ImagePtr image = nullptr;
//     rhi::SamplerValue sampler = {};

//     const rhi::ImageInfo*  getInfo() const noexcept {
//         if (image) {
//             return &image->getInfo();
//         }
//         return nullptr;
//     }
// };

}