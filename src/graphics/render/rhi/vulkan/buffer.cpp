#include "buffer.h"

#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "vk_types.h"

#include "log/log.h"

namespace engine::vulkan {

    Buffer::Buffer(Device & device, const rhi::BufferInfo & info, size_t size) : _device(&device), _size(size) {
        const VkBufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(info.flags),
            .size = size,
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
        if (_buffer) {
            _device->getMemoryAllocator().destroyBuffer(_buffer, _allocation);
        }

        const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
        for (auto & [_, bufferView] : _bufferViews) {
            vkDestroyBufferView(_device->operator()(), bufferView, allocationCallbacks);
        }
    }

    VkBufferView Buffer::recieveView(rhi::Format format) noexcept {
        // todo: use composite key format + offset + range
        if (auto it = _bufferViews.find(format); it != _bufferViews.end()) {
            return it->second;
        }

        VkBufferViewCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .buffer = _buffer,
            .format = utils::convert2vk(format),
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        };

        VkBufferView bufferView = VK_NULL_HANDLE;
        const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
        if (vkCreateBufferView(_device->operator()(), &info, allocationCallbacks, &bufferView) == VK_SUCCESS) {
            _bufferViews.emplace(format, bufferView);
        }
        
        return bufferView;   
    }

    VkBufferMemoryBarrier Buffer::transition(VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags, 
        size_t offset, size_t size, uint32_t queueFamily) noexcept {
        auto const currentFamily = _queueFamilyIndex;
        _queueFamilyIndex = queueFamily;
        return VkBufferMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = srcAccessFlags,
            .dstAccessMask = dstAccessFlags,
            .srcQueueFamilyIndex = currentFamily,
            .dstQueueFamilyIndex = queueFamily,
            .buffer = _buffer,
            .offset = offset,
            .size = size
        };
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
    
    std::string_view Buffer::getName() const noexcept {
        if (_allocation.info.pName) {
            return _allocation.info.pName;
        }
        return {};
    }
}