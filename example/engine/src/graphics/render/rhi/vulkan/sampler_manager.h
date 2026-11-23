#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/types.h"
#include "stl/unordered_map.h"

#include <vulkan/vulkan.h>

namespace engine::vulkan {

    class Device;

    class SamplerManager {
    public:
        SamplerManager(const Device & device) : _device(device) {}
        void destroy() noexcept;

        VkSampler getSampler(const rhi::SamplerInfo & samplerInfo);
        
    private:
        // need synchronisations ?
        using Samplers = stl::unordered_map<rhi::SamplerInfo, VkSampler, 
                                            hash::Hasher<rhi::SamplerInfo>, std::equal_to<>>;


        const Device & _device;
        Samplers _samplers;
    };

}