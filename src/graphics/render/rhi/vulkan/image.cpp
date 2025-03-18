#include "image.h"

#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "vk_types.h"

#include "log/log.h"

namespace engine::vulkan {

Image::Image(Device & device, const rhi::ImageInfo & info) : 
    _device(device), 
    _info(info),
    _imageSubStates(_info.arrayLayers * _info.mipLevels) {

    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = utils::convert2vk(_info.flags),
        .imageType = utils::convert2vk(_info.type),
        .format = utils::convert2vk(_info.format),
        .extent = utils::convert2vk(_info.extent),
        .mipLevels = _info.mipLevels,
        .arrayLayers = _info.arrayLayers,
        .samples = utils::convert2vk(_info.samples),
        .tiling = utils::convert2vk(_info.tiling),
        .usage = utils::convert2vk(_info.usage),
        .sharingMode = utils::convert2vk(_info.sharing),
        .queueFamilyIndexCount = static_cast<uint32_t>(_info.queueFamilyIndices.size()),
        .pQueueFamilyIndices = _info.queueFamilyIndices.data(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    if (auto result = _device.getMemoryAllocator().createImage(imageInfo, utils::convert2vk(_info.memoryFlags), info.memoryExtFlags, _image, _allocation); result != VK_SUCCESS) {
        LOG_E("vulkan: vkCreateImage error: %d", result);
        return;
    }
}

Image::Image(Device & device, const rhi::ImageInfo & info, VkImage externalImage) :
    _device(device), 
    _image(externalImage),
    _info(info),
    _imageSubStates(_info.arrayLayers * _info.mipLevels),
    _externalImage(true)  {

    }

Image::~Image() {
    if (_image && !_externalImage) {
        _device.getMemoryAllocator().destroyImage(_image, _allocation);
    }

    const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
    for (auto it = _imageViews.begin(); it != _imageViews.end(); ++it) {
        vkDestroyImageView(_device(), it->second, allocationCallbacks);
    }
}

VkImageView Image::getImageView(rhi::ImageViewType viewType, 
                               uint32_t baseLayer, uint32_t layersCount, 
                               uint32_t baseLevel, uint32_t levelsCount, 
                               rhi::ImageAspectFlags aspectFlags) {
    const ImageViewDescription description {
        ImageSubresourceRange{
            static_cast<uint8_t>(baseLevel),
            static_cast<uint8_t>(levelsCount),
            static_cast<uint8_t>(baseLayer),
            static_cast<uint8_t>(layersCount)
        },
        viewType, aspectFlags
    };

    auto it = _imageViews.find(description);
    if (it != _imageViews.end()) {
        return it->second;
    }

    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = _image,
        .viewType = utils::convert2vk(viewType),
        .format = utils::convert2vk(_info.format),
        .components = rhi::isDepthFormat(_info.format) ? VkComponentMapping {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        } : VkComponentMapping {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A
        },
        .subresourceRange = {
            .aspectMask = utils::convert2vk(_info.aspectFlags & aspectFlags),
            .baseMipLevel = baseLevel,
            .levelCount = levelsCount,
            .baseArrayLayer = baseLayer,
            .layerCount = layersCount
        }
    };

    VkImageView imageView;
    const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
    if (auto result = vkCreateImageView(_device(), &viewInfo, allocationCallbacks, &imageView); 
        result != VK_SUCCESS) {
        LOG_E("vulkan: vkCreateImageView error: %d", result);
        return VK_NULL_HANDLE;
    }

  return _imageViews.insert({description, imageView}).first->second;
}

void Image::assignLayout(VkImageLayout dstLayout, uint8_t baseLayer, uint8_t layersCount, 
                    uint8_t baseLevel, uint8_t levelsCount) noexcept {
    for (size_t i = baseLayer; i < baseLayer + layersCount; ++i) {
        for (size_t j = baseLevel; j < baseLevel + levelsCount; ++j) {
            auto & subState = _imageSubStates[i * _info.mipLevels + j];
            subState.layout = dstLayout;
        }
    }
}

std::vector<ImageMemoryBarrierInfo> Image::changeLayout(VkImageLayout dstLayout, 
                        VkPipelineStageFlags srcStage,
                        VkPipelineStageFlags dstStage, 
                        VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags,
                        uint32_t queueFamily, uint8_t baseLayer, uint8_t layersCount, 
                        uint8_t baseLevel, uint8_t levelsCount) {
    //todo! check this function

    VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.image = _image;
    barrier.srcAccessMask = srcAccessFlags;
    barrier.dstAccessMask = dstAccessFlags;
    barrier.newLayout = dstLayout;
    barrier.dstQueueFamilyIndex = queueFamily;

    VkImageSubresourceRange & subresourceRange = barrier.subresourceRange;
    subresourceRange.aspectMask = utils::convert2vk(_info.aspectFlags);

    std::vector<ImageMemoryBarrierInfo> barriers;

    if (_imageStatesIdentical) {
        auto & subState = _imageSubStates[0u];
        barrier.oldLayout = subState.layout;
        barrier.srcQueueFamilyIndex = subState.queueFamilyIndex;

        subresourceRange.baseArrayLayer = baseLayer;
        subresourceRange.layerCount = layersCount;
        subresourceRange.baseMipLevel = baseLevel;
        subresourceRange.levelCount = levelsCount;

        barriers.emplace_back(srcStage, dstStage, barrier);

        for (size_t i = baseLayer; i < baseLayer + layersCount; ++i) {
            for (size_t j = baseLevel; j < baseLevel + levelsCount; ++j) {
                auto & subState = _imageSubStates[i * _info.mipLevels + j];
                subState.layout = dstLayout;
                subState.queueFamilyIndex = queueFamily;
            }
        }
    } else {
        struct LevelsInfo {
            uint8_t start;
            uint8_t count = 1u;
            ImageSubState subState;

            inline bool operator == (const LevelsInfo& other) const noexcept = default;

            LevelsInfo(uint8_t start, uint8_t count, ImageSubState subState) : start(start), count(count), subState(subState) {}
        };

        struct LayerInfo {
            std::vector<LevelsInfo> levels;
            uint8_t start;
            uint8_t count = 1u;

            LayerInfo() = default;
            LayerInfo(LayerInfo && ) noexcept = default;
        };

        using LayersInfo = std::vector<LayerInfo>;
        LayersInfo layers;

        size_t barriersCount = 0u;

        for (size_t i = baseLayer; i < baseLayer + layersCount; ++i) {
            uint32_t srcQueueFamily = queueFamily;
            VkImageLayout srcLayout = dstLayout;

            LayerInfo layer;
            layer.start = i;
            for (size_t j = baseLevel; j < baseLevel + levelsCount; ++j) {
                auto & subState = _imageSubStates[i * _info.mipLevels + j];
                if (subState.layout != srcLayout || subState.queueFamilyIndex != srcQueueFamily) {
                    srcQueueFamily = subState.queueFamilyIndex;
                    srcLayout = subState.layout;
                    layer.levels.emplace_back(uint8_t(j), uint8_t(1u), subState);

                    subState.layout = dstLayout;
                    subState.queueFamilyIndex = queueFamily;
                } else {
                    subState.layout = dstLayout;
                    subState.queueFamilyIndex = queueFamily;
                    if (!layer.levels.empty()) {
                        ++layer.levels.back().count;
                    }
                }
            }

            if (!layer.levels.empty()) {
                if (!layers.empty()) {
                    if (layers.back().levels == layer.levels && 
                        layers.back().start + layers.back().count == i) { // слои идут подряд
                        ++layers.back().count;
                    } else {
                        ++barriersCount;
                        layers.emplace_back(std::move(layer));
                    }
                } else {
                    ++barriersCount;
                    layers.emplace_back(std::move(layer));
                }
            }
        }

        barriers.reserve(barriersCount);

        for (const auto & layersInfo : layers) {
            for (const auto & levelsInfo : layersInfo.levels) {
                auto & subState = levelsInfo.subState;
                barrier.oldLayout = subState.layout;
                barrier.srcQueueFamilyIndex = subState.queueFamilyIndex;

                subresourceRange.baseArrayLayer = layersInfo.start;
                subresourceRange.layerCount = layersInfo.count;
                subresourceRange.baseMipLevel = levelsInfo.start;
                subresourceRange.levelCount = levelsInfo.count;

                barriers.emplace_back(srcStage, dstStage, barrier);
            }
        }
    }

    const bool newStatesIdentical = baseLayer == 0u && (layersCount >= _info.arrayLayers) && 
                                    baseLevel == 0u && (levelsCount >= _info.mipLevels);

    if (newStatesIdentical) {
        _imageStatesIdentical = true;
    } else {
        _imageStatesIdentical = true;
        const auto & state0 = _imageSubStates[0u];
        for (size_t i = 1u; i < _imageSubStates.size(); ++i) {
            if (_imageSubStates[i] != state0) {
                _imageStatesIdentical = false;
                break;
            }
        }
    }

    return barriers;
}

}