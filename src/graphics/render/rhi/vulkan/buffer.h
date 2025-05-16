#pragma once

#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/runtime.h"
#include "memory_allocator.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace engine::vulkan {

    class Device;
    class PipelineBarriers;

    class Buffer final : public rhi::runtime::Buffer {
    public:
        Buffer() = default;
        Buffer(Device & device, const rhi::BufferInfo & info, size_t sizeInBytes);
        ~Buffer();
        
        Buffer(const Buffer &) = delete;
        Buffer & operator= (const Buffer &) = delete;

        Buffer(Buffer && other) noexcept : rhi::runtime::Buffer(other._size), _device(other._device), 
            _buffer(other._buffer), _allocation(other._allocation), _bufferViews(std::move(other._bufferViews)),
            _queueFamilyIndex(other._queueFamilyIndex), _usage(other._usage) {
            other._device = nullptr;
            other._buffer = VK_NULL_HANDLE;
            other._allocation = {};
        }

        Buffer & operator= (Buffer && other) noexcept {
            _device = other._device;
            _buffer = other._buffer; 
            _allocation = other._allocation;
            _bufferViews = std::move(other._bufferViews);
            _queueFamilyIndex = other._queueFamilyIndex;
            _size = other._size;
            _usage = other._usage;

            other._device = nullptr;
            other._buffer = VK_NULL_HANDLE;
            other._allocation = {};
            return *this;
        }

        inline explicit operator bool() const noexcept { return _buffer != VK_NULL_HANDLE; }
        inline VkBuffer operator ()() const noexcept { return _buffer; }

        void * map() noexcept override;
        void unmap() noexcept override;
        void flush(size_t offset = 0u, size_t size = VK_WHOLE_SIZE) noexcept override;
        void invalidate(size_t offset = 0u, size_t size = VK_WHOLE_SIZE) noexcept override;
        bool isHostVisible() const noexcept override {
            return memoryFlags() & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }

        template <typename T>
        inline T * map2() noexcept { return static_cast<T*>(map()); }
        
        void setName(std::string_view name) noexcept;
        std::string_view getName() const noexcept;

        VkBufferView recieveView(rhi::Format format) noexcept;
        VkBufferView getView(rhi::Format format) const noexcept { 
            if (auto it = _bufferViews.find(format); it != _bufferViews.end()) {
                return it->second;
            }
            return VK_NULL_HANDLE;
        }
        
        VkMemoryPropertyFlags memoryFlags() const noexcept;
        rhi::BufferUsageFlags usage() const noexcept { return _usage; }
        VkPipelineStageFlags getStage() const noexcept { return _usedStage; }

        void transition(PipelineBarriers & barriers, VkPipelineStageFlags stage, VkAccessFlags dstAccessFlags, 
            size_t offset = 0u, size_t size = VK_WHOLE_SIZE, uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED) noexcept;


    private:
        Device * _device = nullptr;
        VkBuffer _buffer = VK_NULL_HANDLE;
        MemoryAllocation _allocation{};
        std::unordered_map<rhi::Format, VkBufferView> _bufferViews;
        uint32_t _queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        rhi::BufferUsageFlags _usage = rhi::BufferUsageFlags::eNone;

        VkPipelineStageFlags _usedStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkAccessFlags _usedAccess = 0;
    };
}