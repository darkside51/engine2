#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include <memory>
#include <vector>

namespace engine::graphics {

    class InflightBuffer {
    public:
        ~InflightBuffer();

        InflightBuffer(const std::unique_ptr<rhi::runtime::Rhi> & api,
            std::unique_ptr<rhi::runtime::Device> & device, const rhi::BufferInfo & info, size_t size);

        void * map() noexcept;
        void unmap() noexcept;

        template <typename T>
        inline T * map2() noexcept { return static_cast<T*>(map()); }

        void apply(rhi::runtime::CommandListPtr & cmd, uint8_t idx) noexcept;

        rhi::runtime::BufferPtr & operator()(uint8_t idx) noexcept {
            return _buffers[idx].buffer;
        }

        const rhi::runtime::BufferPtr & operator()(uint8_t idx) const noexcept {
            return _buffers[idx].buffer;
        }

        explicit operator bool() const noexcept { return !_buffers.empty(); }

    private:
        struct Data {
            rhi::runtime::BufferPtr buffer = nullptr;
            bool dirty = false;
        };
        std::vector<Data> _buffers;
        void * _tmpMemory = nullptr;
    
        const std::unique_ptr<rhi::runtime::Rhi> & _api;
        std::unique_ptr<rhi::runtime::Device> & _device;
    };
}