#include "staging_buffer.h"

namespace engine::graphics {
    StagingBuffer::StagingBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, size_t size, rhi::MemoryPropertyFlags memoryFlags) {
        _buffers.reserve(device->inFlightFramesCount());

        const rhi::BufferInfo info {
            rhi::BufferUsageFlags::eTransfer_Src,
            memoryFlags
        };

        for (uint8_t i = 0u; i < device->inFlightFramesCount(); ++i) {
            _buffers.emplace_back(api->createBuffer(device, info, size), 0u);
        }
    }

    void StagingBuffer::map(uint8_t idx) noexcept {
        if (_currentFrame != idx) {
            _currentFrame = idx;
            _buffers[idx].offset = 0u;
        }
        _mappedMemory = _buffers[idx].buffer->map2<std::byte>();
    }

    void StagingBuffer::unmap() noexcept {
        _buffers[_currentFrame].buffer->unmap();        
        _mappedMemory = nullptr;
    }

    uint32_t StagingBuffer::requestBytes(uint8_t idx, uint32_t bytes) noexcept { 
        auto & buffer = _buffers[idx];
        if (buffer.offset + bytes >= buffer.buffer->sizeInBytes()) {
            return kNotEnoughMemory;
        }

        auto const result = buffer.offset;
        buffer.offset += bytes;
        return result;
     }

    std::pair<rhi::runtime::BufferPtr *, std::byte*> StagingBuffer::getBuffer(uint8_t idx) noexcept { 
        return {&_buffers[idx].buffer, _mappedMemory};
    }
}