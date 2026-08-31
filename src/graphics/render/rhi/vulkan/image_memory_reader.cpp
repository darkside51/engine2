#include "image_memory_reader.h"
#include "graphics/render/rhi/common/command_list.h"

namespace engine::vulkan {

    ImageMemoryReader::ImageMemoryReader(Device & device, rhi::Format format, rhi::Extent3D extent, uint8_t imagesCount) : 
        _device(device),
        _images(imagesCount) {

        auto imageInfo = rhi::ImageInfo {
            rhi::ImageUsageFlags::eTransfer_Dst,
            format, extent
        };
        imageInfo.memoryFlags = engine::rhi::MemoryPropertyFlags::eHostVisible | engine::rhi::MemoryPropertyFlags::eHostCoherent;

        for (auto & img : _images) {
            img = rhi::runtime::sh_ptr<Image>::make(_device, imageInfo);
        }
    }

    void ImageMemoryReader::prepareMemory(rhi::runtime::CommandList & commandList, rhi::runtime::ImagePtr image, 
        uint8_t frameId, uint8_t mipLevel, uint8_t baseArrayLayer) noexcept {
        const rhi::CopyImage2ImageInfo info = {
            .srcSubresource = rhi::ImageSubresourceLayers{
                .aspectMask = rhi::ImageAspectFlags::eColor,
                .mipLevel = mipLevel,
                .baseArrayLayer = baseArrayLayer,
                .layerCount = 1u
            },
            .dstSubresource = rhi::ImageSubresourceLayers{
                .aspectMask = rhi::ImageAspectFlags::eColor,
                .mipLevel = mipLevel,
                .baseArrayLayer = baseArrayLayer,
                .layerCount = 1u
            },
            .srcOffset = rhi::Offset3D{0, 0, 0},
            .dstOffset = rhi::Offset3D{0, 0, 0},
            .extent = _images[frameId]->getExtent()
        };

        commandList.copyImage2Image(image, _images[frameId], info);
    }

}