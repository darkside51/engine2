#pragma once

#include "graphics/render/rhi/common/types.h"
#include "log/log.h"

#ifdef DEBUG
//#define VMA_DEBUG_LOG_FORMAT(format, ...) LOG_I((format), __VA_ARGS__);
#endif

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
    uint32_t preferredLargeHeapBlockSize = 256u * 1024u * 1024u; // 256Mb
    const VkAllocationCallbacks* allocationCallbacks = nullptr;
};

struct MemoryAllocation {
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
};

class GPUMemoryAllocator {
    struct Properties {
        bool allowLazilyAllocations = false;
    };

public:
    ~GPUMemoryAllocator();
    void create(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, const AllocatorCreateInfo & info);
    void destroy();
    
    VkResult createBuffer(const VkBufferCreateInfo & info, VkMemoryPropertyFlags requiredFlags, 
        rhi::MemoryExtendedFlags memoryExtFlags, VkBuffer & buffer, MemoryAllocation & allocation);

    void destroyBuffer(VkBuffer & buffer, MemoryAllocation & allocation);

    VkResult createImage(const VkImageCreateInfo & info, VkMemoryPropertyFlags requiredFlags, 
        rhi::MemoryExtendedFlags memoryExtFlags, VkImage & image, MemoryAllocation & allocation);
        
    void destroyImage(VkImage & image, MemoryAllocation & allocation);

    void * mapMemory(const MemoryAllocation & allocation) noexcept;
    void unmapMemory(const MemoryAllocation & allocation) noexcept;

    VkResult flushMemory(const MemoryAllocation & allocation, VkDeviceSize offset = 0u, VkDeviceSize size = VK_WHOLE_SIZE) noexcept;
    VkResult invalidateMemory(const MemoryAllocation & allocation, VkDeviceSize offset = 0u, VkDeviceSize size = VK_WHOLE_SIZE) noexcept;

private:
    void initProperties() noexcept;
    VmaAllocator _allocator = VK_NULL_HANDLE;
    Properties _properties;
};

}