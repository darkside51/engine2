#include "inflight_buffer.h"
#include "staging_buffer.h"

#include "graphics/render/rhi/common/command_list.h"

namespace engine::graphics {

    InflightBuffer::~InflightBuffer() {
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

        _tmpMemory = std::make_unique_for_overwrite<std::byte[]>(size);
    }

    void InflightBuffer::resize(const rhi::BufferInfo & info, size_t size) {
        const auto oldSize = static_cast<size_t>(_buffers[0u].buffer->sizeInBytes());

        for (uint8_t i = 0u; i < _device->inFlightFramesCount(); ++i) {
            _buffers[i] = {_api->createBuffer(_device, info, size), false};
        }

        auto newMem = std::make_unique_for_overwrite<std::byte[]>(size);
        memcpy(newMem.get(), _tmpMemory.get(), std::min(oldSize, size));
        _tmpMemory = std::move(newMem);
    }

    void * InflightBuffer::map() noexcept {
        return _tmpMemory.get();
    }

    void InflightBuffer::unmap() noexcept {
        for (auto & [_, dirty]  : _buffers) {
            dirty = true;
        }
    }

    void InflightBuffer::commit(rhi::runtime::CommandListPtr & cmd, uint8_t idx) noexcept { // can use another command buffer if need to call this function in renderpass time!
        // or can create buffers here?
        
        if (auto & [buffer, dirty] = _buffers[idx]; dirty) {
            if (buffer->isHostVisible()) { // it is hostVisible
                auto * memory = buffer->map();
                memcpy(memory, _tmpMemory.get(), buffer->sizeInBytes());
                buffer->flush();
                buffer->unmap();
            } else { // need use stagingBuffer
                if (_stagingBuffer) {
                    const auto offset = _stagingBuffer->requestBytes(idx, buffer->sizeInBytes());
                    if (offset != StagingBuffer::kNotEnoughMemory) {
                        auto [stagingBuffer, memory] = _stagingBuffer->getBuffer(idx);
                        {
                            //auto * memory = stagingBuffer->map2<std::byte>();
                            memcpy(memory + offset, _tmpMemory.get(), buffer->sizeInBytes());
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
                    memcpy(memory, _tmpMemory.get(), buffer->sizeInBytes());
                    stagingBuffer->flush();
                    stagingBuffer->unmap();
                }

                cmd->copyBuffer2Buffer(stagingBuffer, buffer, {.srcOffset = 0u, .dstOffset = 0u, .size = buffer->sizeInBytes()});
            }

            dirty = false;
        }
    }
}