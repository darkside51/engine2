#include "buffer.h"

#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "barriers.h"
#include "vk_types.h"

#include "log/log.h"

namespace engine::vulkan {

    Buffer::Buffer(Device & device, const rhi::BufferInfo & info, size_t sizeInBytes) : 
        rhi::runtime::Buffer(sizeInBytes), 
        _device(&device), 
        _usage(info.usage) {
        const VkBufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(info.flags),
            .size = sizeInBytes,
            .usage = utils::convert2vk(info.usage),
            .sharingMode = utils::convert2vk(info.sharing),
            .queueFamilyIndexCount = static_cast<uint32_t>(info.queueFamilyIndices.size()),
            .pQueueFamilyIndices = info.queueFamilyIndices.data()
        };

        if (auto result = _device->getMemoryAllocator().createBuffer(createInfo, utils::convert2vk(info.memoryFlags), 
            info.memoryExtFlags, _buffer, _allocation); result != VK_SUCCESS) {
            LOG_E("vulkan: vkCreateBuffer error: %d", result);
            return;
        }
    }

    Buffer::~Buffer() {
        if (!_device) {
            return;
        }

        if (_buffer) {
            _device->getMemoryAllocator().destroyBuffer(_buffer, _allocation);
        }

        const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
        for (auto & [_, bufferView] : _bufferViews) {
            vkDestroyBufferView(_device->operator()(), bufferView, allocationCallbacks);
        }
    }

    VkBufferView Buffer::recieveView(rhi::Format format, uint32_t offset, uint32_t range) noexcept {
        // todo: use composite key format + offset + range
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (auto it = _bufferViews.find(format); it != _bufferViews.end()) {
            return it->second;
        }

        VkBufferViewCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .buffer = _buffer,
            .format = utils::convert2vk(format),
            .offset = offset,
            .range = range == rhi::kWholeSize32 ? VK_WHOLE_SIZE : range
        };

        VkBufferView bufferView = VK_NULL_HANDLE;
        const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
        if (vkCreateBufferView(_device->operator()(), &info, allocationCallbacks, &bufferView) == VK_SUCCESS) {
            _bufferViews.emplace(format, bufferView);
        }
        
        return bufferView;   
    }

    void Buffer::flush(size_t offset, size_t size) noexcept {
        _device->getMemoryAllocator().flushMemory(_allocation, offset, size);
    }

    void Buffer::invalidate(size_t offset, size_t size) noexcept {
        _device->getMemoryAllocator().invalidateMemory(_allocation, offset, size);
    }

    void * Buffer::map() noexcept {
        return _device->getMemoryAllocator().mapMemory(_allocation);
    }

    void Buffer::unmap() noexcept {
        _device->getMemoryAllocator().unmapMemory(_allocation);
    }

    void Buffer::setName(std::string_view name) noexcept {
        _allocation.info.pName = name.data();
    }
    
    VkMemoryPropertyFlags Buffer::memoryFlags() const noexcept {
        return _device->gpu().memoryProperties().memoryTypes[_allocation.info.memoryType].propertyFlags;
    }

    std::string_view Buffer::getName() const noexcept {
        if (_allocation.info.pName) {
            return _allocation.info.pName;
        }
        return {};
    }

    void Buffer::transition(PipelineBarriers & barriers, VkPipelineStageFlags stage, VkAccessFlags dstAccessFlags, 
            size_t offset, size_t size, uint32_t queueFamily) noexcept {
        if (_transitionState.usedStage != stage || _transitionState.usedAccess != dstAccessFlags || _queueFamilyIndex != queueFamily || 
            _transitionState.range.min > offset || _transitionState.range.max < offset + size) {

            barriers.insertBufferBarrier(_transitionState.usedStage, stage, VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT, {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = _transitionState.usedAccess,
                .dstAccessMask = dstAccessFlags,
                .srcQueueFamilyIndex = _queueFamilyIndex,
                .dstQueueFamilyIndex = queueFamily,
                .buffer = _buffer,
                .offset = offset,
                .size = size
            });

            _queueFamilyIndex = queueFamily;
            _transitionState.usedStage = stage;
            _transitionState.usedAccess = dstAccessFlags;
            _transitionState.range.min = std::min(offset, _transitionState.range.min);
            _transitionState.range.max = std::max(offset + size, _transitionState.range.max);
        }
    }
}