#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/runtime.h"
#include "memory_allocator.h"
#include "stl/unordered_map.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string_view>

namespace engine::vulkan {

    class Device;
    class PipelineBarriers;

    class Buffer final : public rhi::runtime::Buffer {
        struct BufferViewKey {
            rhi::Format format = rhi::Format::eUndefined;
            uint64_t offset = 0u;
            uint64_t range = rhi::kWholeSize64;

            inline size_t hash() const noexcept {
                return hash::hash_combine(format, offset, range);
            }
            inline bool operator == (const BufferViewKey & other) const noexcept = default;
        };

        using BufferViewsMap = stl::unordered_map<BufferViewKey, VkBufferView, hash::Hasher<BufferViewKey>, std::equal_to<>>;

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

        VkBufferView recieveView(rhi::Format format, uint64_t offset = 0u, uint64_t range = rhi::kWholeSize64) noexcept;
        VkBufferView getView(rhi::Format format, uint64_t offset = 0u, uint64_t range = rhi::kWholeSize64) const noexcept { 
            if (auto it = _bufferViews.find({format, offset, range}); it != _bufferViews.end()) {
                return it->second;
            }
            return VK_NULL_HANDLE;
        }
        
        VkMemoryPropertyFlags memoryFlags() const noexcept;
        rhi::BufferUsageFlags usage() const noexcept { return _usage; }
        VkPipelineStageFlags getStage() const noexcept { return _transitionState.usedStage; }

        void transition(PipelineBarriers & barriers, VkPipelineStageFlags stage, VkAccessFlags dstAccessFlags, 
            size_t offset = 0u, size_t size = VK_WHOLE_SIZE, uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED) noexcept;

    private:
        Device * _device = nullptr;
        VkBuffer _buffer = VK_NULL_HANDLE;
        MemoryAllocation _allocation{};
        // stl::unordered_map<rhi::Format, VkBufferView> _bufferViews;
        BufferViewsMap _bufferViews;

        uint32_t _queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        rhi::BufferUsageFlags _usage = rhi::BufferUsageFlags::eNone;

        struct TransitionState {
            struct Range {
                size_t min = 0u;
                size_t max = 0u;
            };
            VkPipelineStageFlags usedStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkAccessFlags usedAccess = 0;
            Range range{};
        };

        TransitionState _transitionState{};
    };
}