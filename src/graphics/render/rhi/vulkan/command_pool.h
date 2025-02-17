#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.hpp>

#include <array>
#include <vector>

namespace engine::vulkan {

class Device;

class CommandPool {
public:
    CommandPool(const Device & device, uint8_t queueFlags, rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer);
    ~CommandPool();

    VkCommandBuffer allocCommandBuffer(rhi::CommandBufferLevel level);
    void deallocCommandBuffer(rhi::CommandBufferLevel level, VkCommandBuffer commandBuffer);

    inline VkCommandPool operator ()() const noexcept { return _pool; }
    inline explicit operator bool() const noexcept { return _pool != VK_NULL_HANDLE; }

private:
    VkCommandPool _pool = VK_NULL_HANDLE;
    const Device & _device;
    rhi::CommandPoolCreateFlags _flags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer;
    std::array<std::vector<VkCommandBuffer>, 2u> _freeBuffers; // for primary ans secondary buffers
};

class CommandPoolManager {
public:
private:
};

}