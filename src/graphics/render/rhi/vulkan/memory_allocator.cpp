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

        /// Preferred size of a single `VkDeviceMemory` block to be allocated from large heaps > 1 GiB. Optional.
        /** Set to 0 to use default, which is currently 256 MiB. */
        allocatorCreateInfo.preferredLargeHeapBlockSize = info.preferredLargeHeapBlockSize;

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

    void * GPUMemoryAllocator::mapMemory(const MemoryAllocation & allocation) {
        void * ptr;
        const auto result = vmaMapMemory(_allocator, allocation.allocation, &ptr);
        if (result == VK_SUCCESS) {
            return ptr;
        }

        return nullptr;
    }

    void GPUMemoryAllocator::unmapMemory(const MemoryAllocation & allocation) {
        vmaUnmapMemory(_allocator, allocation.allocation);
    }

    VkResult GPUMemoryAllocator::flushMemory(const MemoryAllocation & allocation, VkDeviceSize offset, VkDeviceSize size ) {
        return vmaFlushAllocation(_allocator, allocation.allocation, offset, size);
    }

    VkResult GPUMemoryAllocator::invalidateMemory(const MemoryAllocation & allocation, VkDeviceSize offset, VkDeviceSize size) {
        return vmaInvalidateAllocation(_allocator, allocation.allocation, offset, size);
    }

    inline VmaAllocationCreateFlags receiveAllocationCreateFlags(rhi::MemoryExtendedFlags memoryExtFlags) noexcept {
        VmaAllocationCreateFlags result = 0;
        
        if (std::to_underlying(memoryExtFlags & rhi::MemoryExtendedFlags::eDedicated)) {
            result |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        if (std::to_underlying(memoryExtFlags & rhi::MemoryExtendedFlags::eTry_No_Allocate_New)) {
            result |= VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT;
        }

        if (std::to_underlying(memoryExtFlags & rhi::MemoryExtendedFlags::eMapped)) {
            result |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        if (std::to_underlying(memoryExtFlags & rhi::MemoryExtendedFlags::eCan_Alias)) {
            result |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
        }
        
        return result;
    }

    VkResult GPUMemoryAllocator::createBuffer(const VkBufferCreateInfo & info, VkMemoryPropertyFlags requiredFlags, 
        rhi::MemoryExtendedFlags memoryExtFlags, VkBuffer & buffer, MemoryAllocation & allocation) {
        VmaAllocationCreateInfo vmaInfo = {};

        vmaInfo.flags = receiveAllocationCreateFlags(memoryExtFlags);
        vmaInfo.usage = VMA_MEMORY_USAGE_AUTO; // VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE or VMA_MEMORY_USAGE_AUTO_PREFER_HOST
        vmaInfo.requiredFlags = requiredFlags;

        if (requiredFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) {
            if (requiredFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT) { 
                // host cached memory type is preferred for random access
                vmaInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            } else {
                // use sequental write for no host cached memory
                vmaInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
        }
         
        return vmaCreateBuffer(_allocator, &info, &vmaInfo, &buffer, &allocation.allocation, &allocation.info);
    }

    void GPUMemoryAllocator::destroyBuffer(VkBuffer & buffer, MemoryAllocation & allocation) {
        vmaDestroyBuffer(_allocator, buffer, allocation.allocation);
    }

    VkResult GPUMemoryAllocator::createImage(const VkImageCreateInfo & info, VkMemoryPropertyFlags requiredFlags, rhi::MemoryExtendedFlags memoryExtFlags,
        VkImage & image, MemoryAllocation & allocation) {
        VmaAllocationCreateInfo vmaInfo = {};

        vmaInfo.flags = receiveAllocationCreateFlags(memoryExtFlags);
        // vmaInfo.flags = ; // check usage for set it
        vmaInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // VMA_MEMORY_USAGE_GPU_ONLY ? // VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE or VMA_MEMORY_USAGE_AUTO_PREFER_HOST
        vmaInfo.requiredFlags = requiredFlags;

        return vmaCreateImage(_allocator, &info, &vmaInfo, &image, &allocation.allocation, &allocation.info);
    }

    void GPUMemoryAllocator::destroyImage(VkImage & image, MemoryAllocation & allocation) {
        vmaDestroyImage(_allocator, image, allocation.allocation);
    }
}