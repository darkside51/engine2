#pragma once

#include "device.h"
#include "image.h"
#include "stl/vector.h"

#include <cstddef>
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

        void getValue(uint16_t x, uint16_t y, uint8_t frameId, void * color, size_t sizeInBytes) noexcept override {
            auto & img = *_images[frameId];
            //const auto & info = img.getInfo();
            //const auto format = info.format;
            //const auto & extent = info.extent;

            VkImageSubresource subresource {
                .aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .arrayLayer = 0
            };
            VkSubresourceLayout layout;
            auto & vulkanImage = static_cast<vulkan::Image&>(img);
            auto vkImage = vulkanImage();

            vkGetImageSubresourceLayout(_device(), vkImage, &subresource, &layout);

            auto * memory = static_cast<std::byte*>(img.map());
            img.invalidate();

            std::memcpy(color, memory + (x * sizeInBytes + y * layout.rowPitch), sizeInBytes);

            img.unmap();
        }

    private:
        Device & _device;
        stl::vector<rhi::runtime::ImagePtr> _images;
    };

}