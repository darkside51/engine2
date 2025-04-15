#pragma once

#include "types.h"

#include <cstdint>



namespace engine::rhi {

namespace runtime {
    class Image;
}

struct FrameBufferAttachment {
    runtime::Image *image = nullptr;
    Extent2D imageSize = {0u, 0u};
    ImageAspectFlags aspect = ImageAspectFlags::eColor;
    uint8_t layer = 0u;
    uint8_t level = 0u;

    // FrameBufferAttachment(FrameBufferAttachment &&) noexcept = default;
    // FrameBufferAttachment(const FrameBufferAttachment &) = default;
};

}