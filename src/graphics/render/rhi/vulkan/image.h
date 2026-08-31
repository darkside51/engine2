#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/runtime.h"

#include "stl/span.h"
#include "stl/unordered_map.h"
#include "stl/utility.h"
#include "stl/vector.h"

#include "memory_allocator.h"

#include <vulkan/vulkan.h>
#include <cstdint>

namespace engine::vulkan {

class Device;
class PipelineBarriers;

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
    VkPipelineStageFlags usedStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags accessFlags = 0;
    uint32_t queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    inline bool operator == (const ImageSubState& other) const noexcept = default;
};

class Image final : public rhi::runtime::Image {
    using ImageViewMap = stl::unordered_map<ImageViewDescription, VkImageView, hash::Hasher<ImageViewDescription>, std::equal_to<>>;
    
public:
    Image();
    Image(Device & device, const rhi::ImageInfo & imageInfo);
    Image(Device & device, const rhi::ImageInfo & imageInfo, VkImage externalImage);
    ~Image();

    Image(const Image &) = delete;
    Image & operator= (const Image &) = delete;

    Image(Image && other) noexcept : rhi::runtime::Image(std::move(other._info)), _device(other._device), _image(other._image), 
            _allocation(other._allocation), 
            _imageViews(std::move(other._imageViews)), _imageSubStates(std::move(other._imageSubStates)),
            _imageStatesIdentical(other._imageStatesIdentical), _externalImage(other._externalImage) {
            other._device = nullptr;
            other._image = VK_NULL_HANDLE;
            other._allocation = {};
        }

    Image & operator= (Image && other) noexcept {
        _device = other._device;
        _image = other._image; 
        _allocation = other._allocation;
        _info = std::move(other._info);
        _imageViews = std::move(other._imageViews);
        _imageSubStates = std::move(other._imageSubStates);
        _imageStatesIdentical = other._imageStatesIdentical;
        _externalImage = other._externalImage;

        other._device = nullptr;
        other._image = VK_NULL_HANDLE;
        other._allocation = {};
        return *this;
    }

    inline explicit operator bool() const noexcept { return _image != VK_NULL_HANDLE; }
    inline VkImage operator ()() const noexcept { return _image; }

    VkImageView getImageView(rhi::ImageViewType viewType, 
                               uint32_t baseLayer, uint32_t layersCount, 
                               uint32_t baseLevel, uint32_t levelsCount, 
                               rhi::ImageAspectFlags aspectFlags);

    void assignLayout(VkImageLayout dstLayout, uint8_t baseLayer, uint8_t baseLevel, 
        uint8_t layersCount = 0u, uint8_t levelsCount = 0u) noexcept;

    stl::span<const ImageSubState> getSubStates() const noexcept { return _imageSubStates; }

    VkImageLayout getLayout(uint8_t layer, uint8_t level) const noexcept;

    void transition(PipelineBarriers & barriers, VkImageLayout dstLayout, 
        VkPipelineStageFlags dstStage, VkAccessFlags dstAccessFlags, 
        uint8_t baseLayer, uint8_t baseLevel, 
        uint8_t layersCount = 0u, uint8_t levelsCount = 0u, 
        uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED);

    void * map() noexcept override;
    void unmap() noexcept override;
    void flush(size_t offset = 0u, size_t size = VK_WHOLE_SIZE) noexcept override;
    void invalidate(size_t offset = 0u, size_t size = VK_WHOLE_SIZE) noexcept override;
    bool isHostVisible() const noexcept override {
        return memoryFlags() & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    VkMemoryPropertyFlags memoryFlags() const noexcept;

private:
    Device * _device = nullptr;
    VkImage _image = VK_NULL_HANDLE;
    MemoryAllocation _allocation = {};

    ImageViewMap _imageViews = {};
    stl::vector<ImageSubState> _imageSubStates = {}; // size must be _layers * _mipLevels
    bool _imageStatesIdentical = true;
    bool _externalImage = false;
};

}