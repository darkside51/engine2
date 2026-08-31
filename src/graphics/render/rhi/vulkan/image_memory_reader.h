#pragma once

#include "device.h"
#include "image.h"
#include "stl/vector.h"

#include <cstdint>

namespace engine::vulkan {

    class ImageMemoryReader {
    public:
        ImageMemoryReader(Device & device, rhi::Format format, rhi::Extent3D extent, uint8_t imagesCount);
        void prepareMemory(rhi::runtime::CommandList & commandList, rhi::runtime::ImagePtr image, 
            uint8_t frameId, uint8_t mipLevel = 0u, uint8_t baseArrayLayer = 0u) noexcept;
        const rhi::runtime::Image & getImage(uint8_t frameId) const noexcept {
            return *_images[frameId];
        }

    private:
        Device & _device;
        stl::vector<rhi::runtime::ImagePtr> _images;
    };

}