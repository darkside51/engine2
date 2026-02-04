#include "inflight_buffer.h"
#include "staging_buffer.h"

#include "graphics/render/rhi/common/command_list.h"

namespace engine::graphics {

    InflightBuffer::~InflightBuffer() {
        _tmpMemory = nullptr;

        for (auto & [mappedMemory, buffer, _] : _buffers) {
            if (mappedMemory != nullptr) {
                buffer->unmap();
            }
        }
    }

    InflightBuffer::InflightBuffer(rhi::runtime::Rhi & api,
        rhi::runtime::Device & device, const rhi::BufferInfo & info, size_t size) :
        _size(size),
        _bufferInfo(info),
        _api(api), 
        _device(device) {
        _buffers.reserve(device.inFlightFramesCount());
        for (uint8_t i = 0u; i < _device.inFlightFramesCount(); ++i) {
            _buffers.emplace_back(nullptr, nullptr, false);
        }

        _tmpMemory = std::make_unique_for_overwrite<std::byte[]>(size);
    }

    void InflightBuffer::resize(size_t size) {
        const auto oldSize = static_cast<size_t>(sizeInBytes());
        if (oldSize == size) {
            return;
        }

        auto newMem = std::make_unique_for_overwrite<std::byte[]>(size);
        memcpy(newMem.get(), _tmpMemory.get(), std::min(oldSize, size));
        _tmpMemory = std::move(newMem);
        _size = size;
    }

    void * InflightBuffer::memory() noexcept {
        return _tmpMemory.get();
    }

    void InflightBuffer::modify(uint32_t bytesCount) noexcept {
        for (auto & [_, __, dirty]  : _buffers) {
            dirty = std::max(bytesCount, dirty); // todo: check offset too!
        }
    }

    void InflightBuffer::applySize(uint8_t idx) noexcept {
        auto & [mappedMemory, buffer, _] = _buffers[idx];
        if (buffer == nullptr || buffer->sizeInBytes() != _size) {
            if (mappedMemory != nullptr) {
                buffer->unmap();
            }
            buffer = _api.createBuffer(_device, _bufferInfo, _size);
        }
    }

    void InflightBuffer::commit(rhi::runtime::CommandList & cmd, uint8_t idx) noexcept { // can use another command buffer if need to call this function in renderpass time!
        if (auto & [mappedMemory, buffer, dirty] = _buffers[idx]; dirty > 0u) {
            const auto bytesChanged = std::min(_size, dirty);
            if (buffer == nullptr || buffer->sizeInBytes() != _size) {
                if (mappedMemory != nullptr) {
                    buffer->unmap();
                }
                buffer = _api.createBuffer(_device, _bufferInfo, _size);
            }

            if (buffer->isHostVisible()) { // it is hostVisible
                if (mappedMemory == nullptr) {
                    mappedMemory = buffer->map();
                }
                memcpy(mappedMemory, _tmpMemory.get(), bytesChanged);
                buffer->flush(); // vma check is memory coherent 
            } else { // need use stagingBuffer
                if (bytesChanged < rhi::runtime::command_limits::kMaxMemoryBytesToCopyCommand) { // use cmdUpdateBuffer
                    cmd.updateBuffer(buffer, 0u, bytesChanged, _tmpMemory.get());
                    dirty = 0u;
                    return;
                }

                if (_stagingBuffer) {
                    const auto offset = _stagingBuffer->requestBytes(idx, bytesChanged);
                    if (offset != StagingBuffer::kNotEnoughMemory) {
                        auto [stagingBuffer, memory] = _stagingBuffer->getBuffer(idx);
                        {
                            //auto * memory = stagingBuffer->map2<std::byte>();
                            memcpy(memory + offset, _tmpMemory.get(), bytesChanged);
                            (*stagingBuffer)->flush(offset, bytesChanged);
                            //stagingBuffer->unmap();
                        }

                        cmd.copyBuffer2Buffer(*stagingBuffer, buffer, {.srcOffset = offset, .dstOffset = 0u, .size = bytesChanged});
                        dirty = 0u;
                        return;
                    }
                }

                const rhi::BufferInfo info {
                    rhi::BufferUsageFlags::eTransfer_Src,
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached
                };
            
                auto stagingBuffer = _api.createBuffer(_device, info, bytesChanged);
                {
                    auto * memory = stagingBuffer->map();
                    memcpy(memory, _tmpMemory.get(), bytesChanged);
                    stagingBuffer->flush(); // vma check is memory coherent
                    stagingBuffer->unmap();
                }

                cmd.copyBuffer2Buffer(stagingBuffer, buffer, {.srcOffset = 0u, .dstOffset = 0u, .size = bytesChanged});
            }

            dirty = 0u;
        }
    }
}