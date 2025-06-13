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

        InflightBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, const rhi::BufferInfo & info, size_t size);

        void setStagingBuffer(InflightStagingBuffer * staging) noexcept { _stagingBuffer = staging; }

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

        bool isHostVisible() const noexcept {
            if (_buffers.empty()) {
                return true;
            }

            return _buffers[0u].buffer->isHostVisible();
        }

    private:
        struct Data {
            rhi::runtime::BufferPtr buffer = nullptr;
            bool dirty = false;
        };
        std::vector<Data> _buffers;
        std::unique_ptr<std::byte[]> _tmpMemory = nullptr;
    
        const std::unique_ptr<rhi::runtime::Rhi> & _api;
        std::unique_ptr<rhi::runtime::Device> & _device;
        InflightStagingBuffer * _stagingBuffer = nullptr;
    };
}