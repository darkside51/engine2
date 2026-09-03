#pragma once

#include "device.h"
#include "image.h"
#include "stl/vector.h"

#include <cstdint>

namespace engine::vulkan {

    class ImageMemoryReader : public rhi::runtime::ImageMemoryReader {
    public:
        ImageMemoryReader(Device & device, rhi::Format format, rhi::Extent3D extent, uint8_t imagesCount);
        ~ImageMemoryReader() = default;

        void prepareMemory(rhi::runtime::CommandList & commandList, rhi::runtime::ImagePtr & image, 
            uint8_t frameId, uint8_t mipLevel = 0u, uint8_t baseArrayLayer = 0u,
            rhi::Offset3D srcOffset = {0, 0, 0}, rhi::Offset3D dstOffset = {0, 0, 0}, rhi::Extent3D extent = {0, 0, 0},
            rhi::ImageAspectFlags aspectMask = rhi::ImageAspectFlags::eColor) noexcept override;
            
        const rhi::runtime::Image & getImage(uint8_t frameId) const noexcept {
            return *_images[frameId];
        }

    private:
        Device & _device;
        stl::vector<rhi::runtime::ImagePtr> _images;
    };

}