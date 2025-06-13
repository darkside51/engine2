#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace engine::graphics {
class StagingBuffer {
    public:
        inline static uint32_t kNotEnoughMemory = 0xff'ff'ff'ffu;

        StagingBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, size_t size, 
            rhi::MemoryPropertyFlags memoryFlags = rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);

        uint32_t requestBytes(uint32_t bytes) noexcept;
        rhi::runtime::BufferPtr & getBuffer() noexcept;

        std::byte* map() noexcept;
        void unmap() noexcept;

        uint32_t sizeInBytes() const noexcept {
            return _data.buffer->sizeInBytes();
        }

        void reset() noexcept;

    private:
        struct Data {
            rhi::runtime::BufferPtr buffer = nullptr;
            uint32_t offset = 0u;
        };
        Data _data;
    };

    class InflightStagingBuffer {
    public:

        InflightStagingBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, size_t size, 
            rhi::MemoryPropertyFlags memoryFlags = rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);

        uint32_t requestBytes(uint8_t idx, uint32_t bytes) noexcept;
        std::pair<rhi::runtime::BufferPtr *, std::byte*> getBuffer(uint8_t idx) noexcept;

        void map(uint8_t idx) noexcept;
        void unmap() noexcept;

        uint32_t sizeInBytes() const noexcept {
            if (_buffers.empty()) {
                return 0u;
            }

            return _buffers[0u].sizeInBytes();
        }

    private:
        std::vector<StagingBuffer> _buffers;

        uint8_t _currentFrame = 0u;
        std::byte* _mappedMemory = nullptr;
    };
}