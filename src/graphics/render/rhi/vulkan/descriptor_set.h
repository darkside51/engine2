#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <array>

namespace engine::vulkan {

    class DescriptorPool;

    struct DescriptorSet {
        DescriptorSet(const DescriptorSet &) = delete;
        DescriptorSet & operator= (const DescriptorSet &) = delete;

        DescriptorSet(DescriptorSet && other) noexcept : pool(other.pool), set(other.set) {
            other.pool = nullptr;
            other.set = VK_NULL_HANDLE;
        }

        DescriptorSet & operator= (DescriptorSet && other) noexcept {
            pool = other.pool;
            set = other.set;
            other.pool = nullptr;
            other.set = VK_NULL_HANDLE;
            return *this;
        }


        DescriptorSet(const rhi::shaders::BindingSetLayout & layout, VkDescriptorSet set, DescriptorPool & pool);
        ~DescriptorSet();

        inline VkDescriptorSet operator ()() const noexcept { return set; }
        inline explicit operator bool() const noexcept { return set != VK_NULL_HANDLE; }

        VkDescriptorSet set = VK_NULL_HANDLE;
        DescriptorPool * pool = nullptr;
        std::array<uint32_t, std::to_underlying(rhi::shaders::ResourceType::eCount)> resources = {};
    };
}