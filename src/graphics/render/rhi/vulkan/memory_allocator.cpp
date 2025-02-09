#define VMA_IMPLEMENTATION
#include "memory_allocator.h"

#include "log/log.h"

namespace engine::vulkan {
    GPUMemoryAllocator::~GPUMemoryAllocator() {
        destroy();
        // vmaCalculateStatistics()
    }

    void GPUMemoryAllocator::create(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, const AllocatorCreateInfo & info) {
        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.physicalDevice = gpu;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.vulkanApiVersion = info.apiVersion;

        if (info.flags.KHR_dedicated_allocation_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        if (info.flags.KHR_bind_memory2_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
        }

    #if !defined(VMA_MEMORY_BUDGET) || VMA_MEMORY_BUDGET == 1
        if (info.flags.EXT_memory_budget_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }
    #endif

        if (info.flags.AMD_device_coherent_memory_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
        }

        if (info.flags.KHR_buffer_device_address_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

    #if !defined(VMA_MEMORY_PRIORITY) || VMA_MEMORY_PRIORITY == 1
        if (info.flags.EXT_memory_priority_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
        }
    #endif

        if (info.flags.KHR_maintenance5_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;
        }

        if (info.flags.KHR_external_memory_win32_enabled) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT;
        }

        allocatorCreateInfo.pAllocationCallbacks = info.allocationCallbacks;

  
        if (const auto result = vmaCreateAllocator(&allocatorCreateInfo, &_allocator); result != VK_SUCCESS) {
            LOG_E("vulkan: memory allocator create error %d", result);
            _allocator = VK_NULL_HANDLE;
        } else {
            LOG_I("vulkan: memory allocator create successful");
        }
    }

    void GPUMemoryAllocator::destroy() {
        if (_allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(_allocator);
            _allocator = VK_NULL_HANDLE;
            LOG_I("vulkan: memory allocator destroyed");
        }
    }


//     static void PrintAllocatorStats()
// {
// #if VMA_STATS_STRING_ENABLED
//     char* statsString = nullptr;
//     vmaBuildStatsString(g_hAllocator, &statsString, true);
//     printf("%s\n", statsString);
//     vmaFreeStatsString(g_hAllocator, statsString);
// #endif
// }

    VkResult GPUMemoryAllocator::createImage(const VkImageCreateInfo & info, VkImage & image, MemoryAllocation & allocation) {
        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        // imageAllocCreateInfo.flags = ; // check usage for set it
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO; // VMA_MEMORY_USAGE_GPU_ONLY ?
        imageAllocCreateInfo.requiredFlags = info.flags;

        return vmaCreateImage(_allocator, &info, &imageAllocCreateInfo, &image, &allocation.allocation, &allocation.info);
    }

    void GPUMemoryAllocator::destroyImage(VkImage & image, MemoryAllocation & allocation) {
        vmaDestroyImage(_allocator, image, allocation.allocation);
    }
}