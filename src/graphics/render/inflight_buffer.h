#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace engine::graphics {

    class StagingBuffer {
    public:
        inline static uint32_t kNotEnoughMemory = 0xff'ff'ff'ffu;

        StagingBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, size_t size, rhi::MemoryPropertyFlags memoryFlags = rhi::MemoryPropertyFlags::eHostVisible);

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

    class InflightBuffer {
    public:
        ~InflightBuffer();

        InflightBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, const rhi::BufferInfo & info, size_t size);

        void setStagingBuffer(StagingBuffer * staging) noexcept { _stagingBuffer = staging; }

        void * map() noexcept;
        void unmap() noexcept;

        template <typename T>
        inline T * map2() noexcept { return static_cast<T*>(map()); }

        void commit(rhi::runtime::CommandListPtr & cmd, uint8_t idx) noexcept;

        rhi::runtime::BufferPtr & operator()(uint8_t idx) noexcept {
            return _buffers[idx].buffer;
        }

        const rhi::runtime::BufferPtr & operator()(uint8_t idx) const noexcept {
            return _buffers[idx].buffer;
        }

        explicit operator bool() const noexcept { return !_buffers.empty(); }

        uint32_t sizeInBytes() const noexcept {
            if (_buffers.empty()) {
                return 0u;
            }

            return _buffers[0u].buffer->sizeInBytes();
        }

    private:
        struct Data {
            rhi::runtime::BufferPtr buffer = nullptr;
            bool dirty = false;
        };
        std::vector<Data> _buffers;
        void * _tmpMemory = nullptr;
    
        const std::unique_ptr<rhi::runtime::Rhi> & _api;
        std::unique_ptr<rhi::runtime::Device> & _device;
        StagingBuffer * _stagingBuffer = nullptr;
    };
}