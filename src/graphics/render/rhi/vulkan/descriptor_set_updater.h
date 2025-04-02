#pragma once

#include "utils/dynamic_array.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <utility>

namespace engine::rhi::shaders {
    struct BindingSetLayout;
}

namespace engine::vulkan {
    
    class Device;
    class Buffer;
    class DescriptorSet;

    class DescriptorSetUpdater {
        inline static constexpr uint8_t kDescriptorsOnStackLimit = 8u;
        inline static constexpr uint8_t kWritesOnStackLimit = 16u;

    public:
        explicit DescriptorSetUpdater(const Device & device, const engine::rhi::shaders::BindingSetLayout & layout);

        void updateBuffers(std::span<const std::pair<Buffer*, uint32_t>> bufferBindings) noexcept;
        void updateImages(std::span<const std::pair<void*, uint32_t>> imageBindings) noexcept;
        void updatetexelBuffers(std::span<const std::pair<void*, uint32_t>> texelBufferBindings) noexcept;

        void flush(const DescriptorSet & set) noexcept;

    private:
        template <typename T>
        struct DescriptorInfo{
            T info;
            uint32_t binding;
        };

        const Device & _device;
        const rhi::shaders::BindingSetLayout & _layout;
        engine::dynamic_array<DescriptorInfo<VkDescriptorBufferInfo>, kDescriptorsOnStackLimit> _buffersInfo;
        engine::dynamic_array<DescriptorInfo<VkDescriptorImageInfo>, kDescriptorsOnStackLimit> _imagesInfo;
        engine::dynamic_array<DescriptorInfo<VkBufferView>, kDescriptorsOnStackLimit> _texelBuffersInfo;
        uint8_t _updateBufferIndex = 0u;
        uint8_t _updateImageIndex = 0u;
        uint8_t _updateTexelBufferIndex = 0u;
    };
    
}