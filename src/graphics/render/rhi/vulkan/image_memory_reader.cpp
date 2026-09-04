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
        imageInfo.memoryFlags = rhi::MemoryPropertyFlags::eHostVisible | 
                                rhi::MemoryPropertyFlags::eHostCoherent;// | 
                                // rhi::MemoryPropertyFlags::eHostCached;
        imageInfo.tiling = rhi::ImageTiling::eLinear;

        for (auto & img : _images) {
            img = rhi::runtime::sh_ptr<Image>::make(_device, imageInfo);
        }
    }

    void ImageMemoryReader::prepareMemory(rhi::runtime::CommandList & commandList, rhi::runtime::ImagePtr & image, 
        uint8_t frameId, uint8_t mipLevel, uint8_t baseArrayLayer, 
        rhi::Offset3D srcOffset, rhi::Offset3D dstOffset, rhi::Extent3D extent,
        rhi::ImageAspectFlags aspectMask) noexcept {
        const rhi::CopyImage2ImageInfo info = {
            .srcSubresource = rhi::ImageSubresourceLayers{
                .aspectMask = aspectMask,
                .mipLevel = mipLevel,
                .baseArrayLayer = baseArrayLayer,
                .layerCount = 1u
            },
            .dstSubresource = rhi::ImageSubresourceLayers{
                .aspectMask = aspectMask,
                .mipLevel = mipLevel,
                .baseArrayLayer = baseArrayLayer,
                .layerCount = 1u
            },
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .extent = ((extent == rhi::Extent3D{0, 0, 0}) ? _images[frameId]->getExtent() : extent)
        };

        commandList.copyImage2Image(image, _images[frameId], info);
        // commandList.prepareImage2HostRead(_images[frameId]);
    }

}