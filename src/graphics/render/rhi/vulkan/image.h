#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/types.h"
#include "memory_allocator.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::vulkan {

class Device;

struct ImageSubresourceRange {
    uint8_t baseMipLevel;
    uint8_t levelCount;
    uint8_t baseArrayLayer;
    uint8_t layerCount;

    inline bool operator == (const ImageSubresourceRange & other) const noexcept = default;
    inline size_t hash() const noexcept {
        return (static_cast<uint32_t>(baseArrayLayer) << 0u) |
               (static_cast<uint32_t>(layerCount) << 8u) |
               (static_cast<uint32_t>(baseMipLevel) << 16u) |
               (static_cast<uint32_t>(levelCount) << 24u);
    }
};

struct ImageViewDescription {
    ImageSubresourceRange subresourceRange;
    rhi::ImageViewType viewType;
    rhi::ImageAspectFlags aspectFlags;
    
    inline bool operator == (const ImageViewDescription& other) const noexcept = default;
    inline size_t hash() const noexcept {
        if constexpr (sizeof(size_t) == 8u) {
            return  static_cast<uint64_t>(subresourceRange.hash()) |
                    (static_cast<uint64_t>(viewType) << 32u) |
                    (static_cast<uint64_t>(aspectFlags) << 40u);
        } else {
            return hash::hash_combine(subresourceRange.hash(), viewType, aspectFlags);
        }
    }
};

struct ImageMemoryBarrierInfo {
    VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage; 
    VkImageMemoryBarrier imageMemoryBarrier;
};

struct ImageSubState {
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    inline bool operator == (const ImageSubState& other) const noexcept = default;
};

class Image {
    using ImageViewMap = std::unordered_map<ImageViewDescription, VkImageView, hash::Hasher<ImageViewDescription>, std::equal_to<>>;
    
public:

    Image(Device & device, const rhi::ImageInfo imageInfo);
    Image(Device & device, const rhi::ImageInfo imageInfo, VkImage externalImage);
    ~Image();

    inline explicit operator bool() const noexcept { return _image != VK_NULL_HANDLE; }
    inline VkImage operator ()() const noexcept { return _image; }

    VkImageView getImageView(rhi::ImageViewType viewType, 
                               uint32_t baseLayer, uint32_t layersCount, 
                               uint32_t baseLevel, uint32_t levelsCount, 
                               rhi::ImageAspectFlags aspectFlags);

    std::vector<ImageMemoryBarrierInfo> changeLayout(VkImageLayout dstLayout, 
                        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, 
                        VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags,
                        uint32_t queueFamily, uint8_t baseLayer, uint8_t layersCount, 
                        uint8_t baseLevel, uint8_t levelsCount);

    void assignLayout(VkImageLayout dstLayout, uint8_t baseLayer, uint8_t layersCount, 
                    uint8_t baseLevel, uint8_t levelsCount) noexcept;

    rhi::Extent3D getExtent() const noexcept { return _info.extent; }

private:
    Device & _device;
    VkImage _image = VK_NULL_HANDLE;
    MemoryAllocation _allocation;

    rhi::ImageInfo _info;

    ImageViewMap _imageViews;
    std::vector<ImageSubState> _imageSubStates; // size must be _layers * _mipLevels
    bool _imageStatesIdentical = true;
    bool _externalImage = false;
};

}