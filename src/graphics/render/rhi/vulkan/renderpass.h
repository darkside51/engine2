#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/renderpass.h"

#include <vulkan/vulkan.h>

#include <unordered_map>

namespace engine::vulkan {

class Device;

class RenderPassManager {
    using RenderPassMap = std::unordered_map<rhi::RenderPassDescription, VkRenderPass, 
                                            hash::Hasher<rhi::RenderPassDescription>, std::equal_to<>>;
public:
    explicit RenderPassManager(const Device & device) : _device(device) {};
    ~RenderPassManager();

    VkRenderPass getRenderPass(const rhi::RenderPassDescription & description);

private:
    const Device & _device;
    RenderPassMap _renderPassCollection;
};

}