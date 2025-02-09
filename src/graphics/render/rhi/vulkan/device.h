#pragma once

#include "graphics/render/rhi/common/device.h"
#include "memory_allocator.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace engine::vulkan {

class PhisicalDevice;

struct DeviceQueueFamily {
    uint32_t index = 0u;
    std::vector<VkQueue> queues;
};

class Device {
public:
    Device(const PhisicalDevice & gpu, const rhi::DeviceCreateInfo & info);
    ~Device();

    inline explicit operator bool() const noexcept { return _device != VK_NULL_HANDLE; }
    inline VkDevice operator ()() const noexcept { return _device; }

    inline const PhisicalDevice & gpu() const noexcept { return _gpu; }

    const std::vector<DeviceQueueFamily> & getQueueFamilies() const noexcept { return _queueFamilies; }
    
    const GPUMemoryAllocator & getMemoryAllocator() const noexcept { return _memoryAllocator; }
    GPUMemoryAllocator & getMemoryAllocator() noexcept { return _memoryAllocator; }

private:
    void createAllocator() noexcept;
    void destroyAllocator() noexcept;

    VkDevice _device = VK_NULL_HANDLE;
    const PhisicalDevice & _gpu;
    std::vector<DeviceQueueFamily> _queueFamilies;
    GPUMemoryAllocator _memoryAllocator;
};

}