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

        uint32_t requestBytes(uint8_t idx, uint32_t bytes) noexcept;
        std::pair<rhi::runtime::BufferPtr *, std::byte*> getBuffer(uint8_t idx) noexcept;

        void map(uint8_t idx) noexcept;
        void unmap() noexcept;

        uint32_t sizeInBytes() const noexcept {
            if (_buffers.empty()) {
                return 0u;
            }

            return _buffers[0u].buffer->sizeInBytes();
        }

    private:
        struct Data {
            rhi::runtime::BufferPtr buffer = nullptr;
            uint32_t offset = 0u;
        };
        std::vector<Data> _buffers;

        uint8_t _currentFrame = 0u;
        std::byte* _mappedMemory = nullptr;
    };
}