#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <unordered_map>

namespace engine::vulkan {
    class Device;

    class DescriptorSetLayoutManager {
        using LayoutsMap = std::unordered_map<rhi::shaders::BindingSetLayout, VkDescriptorSetLayout, 
                                            hash::Hasher<rhi::shaders::BindingSetLayout>, std::equal_to<>>;
    public:
        explicit DescriptorSetLayoutManager(const Device & device) : _device(device) {}
        ~DescriptorSetLayoutManager();
        void destroy() noexcept;

        VkDescriptorSetLayout getSetlayout(const rhi::shaders::BindingSetLayout & key);
        void eraseSetLayout(const rhi::shaders::BindingSetLayout & key) noexcept;

        VkDescriptorSetLayout findSetLayout(const rhi::shaders::BindingSetLayout & key) noexcept {
            if (auto it = _layouts.find(key); it != _layouts.end()) {
                return it->second;
            }
            return VK_NULL_HANDLE;
        }

    private:
        const Device & _device;
        LayoutsMap _layouts;
    };
}