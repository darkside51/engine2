#pragma once

#include "graphics/render/rhi/common/device.h"
#include "memory_allocator.h"
#include "command_pool.h"
#include "renderpass.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace engine::vulkan {

class PhisicalDevice;

struct DeviceQueueFamily {
    uint32_t index = 0u;
    std::vector<VkQueue> queues;
    uint8_t flags = 0u;
};

class Device {
public:
    using RenderPass = VkRenderPass;
    using Fence = VkFence;
    using Semaphore = VkSemaphore;
    
    Device(const PhisicalDevice & gpu, const rhi::DeviceCreateInfo & info);
    ~Device();

    inline explicit operator bool() const noexcept { return _device != VK_NULL_HANDLE; }
    inline VkDevice operator ()() const noexcept { return _device; }

    inline const PhisicalDevice & gpu() const noexcept { return _gpu; }

    const std::vector<DeviceQueueFamily> & getQueueFamilies() const noexcept { return _queueFamilies; }
    const DeviceQueueFamily * findQueueFamily(uint8_t flags, uint32_t start = 0u) const noexcept;
    
    const GPUMemoryAllocator & getMemoryAllocator() const noexcept { return _memoryAllocator; }
    GPUMemoryAllocator & getMemoryAllocator() noexcept { return _memoryAllocator; }

    CommandPool * acquireCommandPool(uint8_t queueFlags, 
        rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer) noexcept {
        return _commandPoolManager.acquirePool(queueFlags, poolFlags);
    }
    CommandPool * acquireCommandPool(uint32_t idx, uint8_t queueFlags, 
        rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer) noexcept {
        return _commandPoolManager.acquirePool(idx, queueFlags, poolFlags);
    }

    RenderPass getRenderPass(const rhi::RenderPassDescription & description) {
        return _renderpassManager.getRenderPass(description);
    }

    Fence createFence(rhi::FenceCreateFlags flags) noexcept;
    Semaphore createSemaphore() noexcept;

private:
    void createAllocator() noexcept;
    void destroyAllocator() noexcept;

    VkDevice _device = VK_NULL_HANDLE;
    const PhisicalDevice & _gpu;
    std::vector<DeviceQueueFamily> _queueFamilies;

    GPUMemoryAllocator _memoryAllocator;
    CommandPoolManager _commandPoolManager;
    RenderPassManager _renderpassManager;
};

}