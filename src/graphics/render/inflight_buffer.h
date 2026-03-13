#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace engine::graphics {

    class InflightStagingBuffer;

    class InflightBuffer {
    public:
        ~InflightBuffer();

        InflightBuffer(rhi::runtime::Rhi & api,
            rhi::runtime::Device & device, const rhi::BufferInfo & info, size_t size);

        void setStagingBuffer(InflightStagingBuffer * staging) noexcept { _stagingBuffer = staging; }

        void * memory() noexcept;
        void modify(uint32_t bytesCount = 0xff'ff'ff'ffu, uint32_t bytesOffset = 0u) noexcept;

        template <typename T>
        inline T * memory2() noexcept { return static_cast<T*>(memory()); }

        void resize(size_t size);

        void applySize(uint8_t idx) noexcept;
        void commit(rhi::runtime::CommandList & cmd, uint8_t idx) noexcept;

        rhi::runtime::BufferPtr & operator()(uint8_t idx) noexcept {
            return _buffers[idx].buffer;
        }

        const rhi::runtime::BufferPtr & operator()(uint8_t idx) const noexcept {
            return _buffers[idx].buffer;
        }

        explicit operator bool() const noexcept { return !_buffers.empty(); }

        uint32_t sizeInBytes() const noexcept {
            return _size;
            // if (_buffers.empty()) {
            //     return 0u;
            // }

            // if (const auto & buffer = _buffers[0u].buffer) {
            //     return buffer->sizeInBytes();
            // }

            // return _size;
        }

        bool isHostVisible() const noexcept {
            if (_buffers.empty()) {
                return true;
            }

            if (const auto & buffer = _buffers[0u].buffer) {
                return buffer->isHostVisible();
            }

            return std::to_underlying(_bufferInfo.memoryFlags & rhi::MemoryPropertyFlags::eHostVisible) != 0u;
        }

    private:
        struct Data {
            std::byte * mappedMemory = nullptr;
            rhi::runtime::BufferPtr buffer = nullptr;
            uint32_t lastDirtyByte = 0u;
            uint32_t offset = 0xff'ff'ff'ffu;
        };
        std::vector<Data> _buffers;
        std::unique_ptr<std::byte[]> _tmpMemory = nullptr;
        uint32_t _size = 0u;
        rhi::BufferInfo _bufferInfo;
    
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;
        InflightStagingBuffer * _stagingBuffer = nullptr;
    };
}