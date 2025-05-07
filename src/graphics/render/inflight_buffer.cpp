#include "inflight_buffer.h"

#include "graphics/render/rhi/common/command_list.h"

namespace engine::graphics {

    InflightBuffer::~InflightBuffer() {
        free(_tmpMemory);
        _tmpMemory = nullptr;
    }

    InflightBuffer::InflightBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
        std::unique_ptr<rhi::runtime::Device> & device, const rhi::BufferInfo & info, size_t size) :
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
                const rhi::BufferInfo info {
                    rhi::BufferUsageFlags::eTransfer_Src,
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached
                };
            
                auto stagingBuffer = _api->createBuffer(_device, info, buffer->sizeInBytes());
                {
                    auto * memory = stagingBuffer->map2<float>();
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