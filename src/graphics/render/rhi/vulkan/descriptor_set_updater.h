#pragma once

#include "utils/dynamic_array.h"
#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <tuple>
#include <utility>

namespace engine::rhi::shaders {
    struct BindingSetLayout;
}

namespace engine::vulkan {
    
    class Device;
    class Buffer;
    class Image;
    class DescriptorSet;

    class DescriptorSetUpdater {
    public:
        inline static constexpr uint8_t kDescriptorsOnStackLimit = 8u;
        inline static constexpr uint8_t kWritesOnStackLimit = 16u;

        enum class BufferUpdateResult : uint8_t {
            eNone = 0u,
            eChangeBufferOrOffset = 1u,
            eChangeDynamicOffset = 2u,
            eChangeBufferAndDynamicOffset = eChangeBufferOrOffset | eChangeDynamicOffset
        };

        DescriptorSetUpdater() = default;
        explicit DescriptorSetUpdater(const Device & device, const engine::rhi::shaders::BindingSetLayout & layout);

        inline explicit operator bool() const noexcept { return _layout != nullptr; }
        inline bool isValid() const noexcept { return _layout != nullptr; }

        void updateBuffers(std::span<const std::pair<Buffer*, uint32_t>> bufferBindings) noexcept;
        void updateImages(std::span<const std::tuple<VkSampler, VkImageView, VkImageLayout, uint32_t>> imageBindings) noexcept;
        void updatetexelBuffers(std::span<const std::tuple<Buffer*, rhi::Format, uint32_t>> texelBufferBindings) noexcept;

        BufferUpdateResult updateBuffer(const Buffer* buffer, uint32_t binding, 
            uint64_t offset = 0u, uint64_t range = rhi::kWholeSize64, bool dynamic = false) noexcept;
        bool updateImage(std::tuple<VkSampler, VkImageView, VkImageLayout> image, uint32_t binding) noexcept;
        bool updateTexelBuffer(Buffer* buffer, rhi::Format format, uint32_t binding, uint64_t offset, uint64_t range) noexcept;

        void flush(const DescriptorSet & set) noexcept;

        void invalidate() noexcept { _layout = nullptr; }

        void fillDynamicOffsets(dynamic_array<uint32_t, 4u * kDescriptorsOnStackLimit> & dynamicOffsets) noexcept;

    private:
        template <typename T>
        struct DescriptorInfo{
            T info;
            uint32_t binding;
        };

        struct DescriptorBufferInfo {
            VkDescriptorBufferInfo vkInfo;
            uint32_t dynamicOffset;
        };

        const Device * _device = nullptr;
        const rhi::shaders::BindingSetLayout * _layout = nullptr;
        engine::dynamic_array<DescriptorInfo<DescriptorBufferInfo>, kDescriptorsOnStackLimit> _buffersInfo;
        engine::dynamic_array<DescriptorInfo<VkDescriptorImageInfo>, kDescriptorsOnStackLimit> _imagesInfo;
        engine::dynamic_array<DescriptorInfo<VkBufferView>, kDescriptorsOnStackLimit> _texelBuffersInfo;
        bool _buffersDirty = false;
        bool _imagesDirty = false;
        bool _texelBufferDirty = false;
        VkDescriptorSet _set = VK_NULL_HANDLE;

        VkDeviceSize _minUniformBufferOffsetAlignment;
        VkDeviceSize _minTexelBufferOffsetAlignment;
        VkDeviceSize _minStorageBufferOffsetAlignment;
    };
    
}