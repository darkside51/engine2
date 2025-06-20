#include "staging_buffer.h"

namespace engine::graphics {
    
    StagingBuffer::StagingBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, size_t size, rhi::MemoryPropertyFlags memoryFlags) {

        const rhi::BufferInfo info {
            rhi::BufferUsageFlags::eTransfer_Src,
            memoryFlags
        };

        _data = {api->createBuffer(device, info, size), 0u};
    }

    void StagingBuffer::reset() noexcept {
        _data.offset = 0u;
    }

    std::byte* StagingBuffer::map() noexcept {
        return _data.buffer->map2<std::byte>();
    }

    void StagingBuffer::unmap() noexcept {
        _data.buffer->unmap();        
    }

    void StagingBuffer::flush() noexcept {
        _data.buffer->flush();
    }

    uint32_t StagingBuffer::requestBytes(uint32_t bytes) noexcept { 
        if (_data.offset + bytes > _data.buffer->sizeInBytes()) {
            return kNotEnoughMemory;
        }

        auto const result = _data.offset;
        _data.offset += bytes;
        return result;
     }

    rhi::runtime::BufferPtr & StagingBuffer::getBuffer() noexcept { 
        return _data.buffer;
    }

    /////////////////

    InflightStagingBuffer::InflightStagingBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, size_t size, rhi::MemoryPropertyFlags memoryFlags) {
        _buffers.reserve(device->inFlightFramesCount());

        for (uint8_t i = 0u; i < device->inFlightFramesCount(); ++i) {
            _buffers.emplace_back(api, device, size, memoryFlags);
        }
    }

    void InflightStagingBuffer::map(uint8_t idx) noexcept {
        if (_currentFrame != idx) {
            _currentFrame = idx;
            _buffers[idx].reset();
        }
        _mappedMemory = _buffers[idx].map();
    }

    void InflightStagingBuffer::unmap() noexcept {
        _buffers[_currentFrame].unmap();        
        _mappedMemory = nullptr;
    }

    uint32_t InflightStagingBuffer::requestBytes(uint8_t idx, uint32_t bytes) noexcept { 
        return _buffers[idx].requestBytes(bytes);
     }

    std::pair<rhi::runtime::BufferPtr *, std::byte*> InflightStagingBuffer::getBuffer(uint8_t idx) noexcept { 
        return {&_buffers[idx].getBuffer(), _mappedMemory};
    }
}