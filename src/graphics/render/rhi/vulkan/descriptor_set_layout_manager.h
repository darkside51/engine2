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

    private:
        LayoutsMap _layouts;
    };
}