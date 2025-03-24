#pragma once

#include "graphics/render/rhi/common/types.h"
#include "memory_allocator.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string_view>

namespace engine::vulkan {

    class Device;

    class Buffer {
    public:
        Buffer(Device & device, const rhi::BufferInfo & info, size_t size);
        ~Buffer();

        VkBufferMemoryBarrier transition(VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags, 
            size_t offset = 0u, size_t size = VK_WHOLE_SIZE, uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED) noexcept;

        inline explicit operator bool() const noexcept { return _buffer != VK_NULL_HANDLE; }
        inline VkBuffer operator ()() const noexcept { return _buffer; }

        void flush(size_t offset = 0u, size_t size = VK_WHOLE_SIZE) noexcept;
        void invalidate(size_t offset = 0u, size_t size = VK_WHOLE_SIZE) noexcept;

        void * map() noexcept;
        template <typename T>
        inline T * map2() noexcept { return static_cast<T*>(map()); }

        void unmap() noexcept;

        size_t getSize() const noexcept { return _allocation.info.size; }

        void setName(std::string_view name) noexcept;
        std::string_view getName() const noexcept;
        
    private:
        Device & _device;
        VkBuffer _buffer = VK_NULL_HANDLE;
        MemoryAllocation _allocation;
        uint32_t _queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    };
}