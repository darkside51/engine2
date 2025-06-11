#include "inflight_buffer.h"

#include "graphics/render/rhi/common/command_list.h"

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
    ///////////////

    InflightBuffer::~InflightBuffer() {
        free(_tmpMemory);
        _tmpMemory = nullptr;
    }

    InflightBuffer::InflightBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device, const rhi::BufferInfo & info, size_t size) :
        _api(api), 
        _device(device) {
        _buffers.reserve(device->inFlightFramesCount());

        for (uint8_t i = 0u; i < _device->inFlightFramesCount(); ++i) {
            _buffers.emplace_back(_api->createBuffer(_device, info, size), false);
        }

        _tmpMemory = std::malloc(size);
    }

    void * InflightBuffer::map() noexcept {
        return _tmpMemory;
    }

    void InflightBuffer::unmap() noexcept {
        for (auto & [_, dirty]  : _buffers) {
            dirty = true;
        }
    }

    void InflightBuffer::commit(rhi::runtime::CommandListPtr & cmd, uint8_t idx) noexcept {
        if (auto & [buffer, dirty] = _buffers[idx]; dirty) {
            if (buffer->isHostVisible()) { // it is hostVisible
                auto * memory = buffer->map();
                memcpy(memory, _tmpMemory, buffer->sizeInBytes());
                buffer->flush();
                buffer->unmap();
            } else { // need use stagingBuffer

                if (_stagingBuffer) {
                    const auto offset = _stagingBuffer->requestBytes(idx, buffer->sizeInBytes());
                    if (offset != StagingBuffer::kNotEnoughMemory) {
                        auto [stagingBuffer, memory] = _stagingBuffer->getBuffer(idx);
                        {
                            //auto * memory = stagingBuffer->map2<std::byte>();
                            memcpy(memory + offset, _tmpMemory, buffer->sizeInBytes());
                            (*stagingBuffer)->flush(offset, buffer->sizeInBytes());
                            //stagingBuffer->unmap();
                        }

                        cmd->copyBuffer2Buffer(*stagingBuffer, buffer, {.srcOffset = offset, .dstOffset = 0u, .size = buffer->sizeInBytes()});
                        dirty = false;
                        return;
                    }
                }

                const rhi::BufferInfo info {
                    rhi::BufferUsageFlags::eTransfer_Src,
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached
                };
            
                auto stagingBuffer = _api->createBuffer(_device, info, buffer->sizeInBytes());
                {
                    auto * memory = stagingBuffer->map();
                    memcpy(memory, _tmpMemory, buffer->sizeInBytes());
                    stagingBuffer->flush();
                    stagingBuffer->unmap();
                }

                cmd->copyBuffer2Buffer(stagingBuffer, buffer, {.srcOffset = 0u, .dstOffset = 0u, .size = buffer->sizeInBytes()});
            }

            dirty = false;
        }
    }
}