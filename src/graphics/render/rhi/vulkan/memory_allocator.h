#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace engine::vulkan {

struct AllocatorCreateInfo {
    struct {
        bool KHR_dedicated_allocation_enabled = false;
        bool KHR_bind_memory2_enabled = false;
        bool KHR_buffer_device_address_enabled = false;
        bool KHR_maintenance5_enabled = false;
        bool KHR_external_memory_win32_enabled = false;
        bool EXT_memory_budget_enabled = false;
        bool EXT_memory_priority_enabled = false;
        bool AMD_device_coherent_memory_enabled = false;
    } flags;

    uint32_t apiVersion = 0u;
    const VkAllocationCallbacks* allocationCallbacks = nullptr;
};

struct MemoryAllocation {
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
};

class GPUMemoryAllocator {
public:
    ~GPUMemoryAllocator();
    void create(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, const AllocatorCreateInfo & info);
    void destroy();
    
    VkResult createImage(const VkImageCreateInfo & info, VkImage & image, MemoryAllocation & allocation);
    void destroyImage(VkImage & image, MemoryAllocation & allocation);

private:
    VmaAllocator _allocator = VK_NULL_HANDLE;
};

}