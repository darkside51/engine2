#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "math/math.h"
#include "stl/vector.h"

#include <cstdint>

namespace engine::graphics {
    
    struct ColorIdsFrameBuffer {
        rhi::runtime::ImagePtr image = nullptr;
        rhi::runtime::FrameBufferPtr frameBuffer = {};
    };

    class ColorPicker {
    public:
        ColorPicker(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
            uint16_t size);

        void createRenderData();
        void beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept;
        void endRenderPass(rhi::runtime::CommandList & commandList) const noexcept;

        const rhi::runtime::ImagePtr & getImage(uint8_t frameId) const noexcept {
            return _framebuffers[frameId].image;
        }

        uint32_t getValue(vec2u coords) const noexcept;

    private:
        stl::vector<ColorIdsFrameBuffer> _framebuffers; // inflight frames count
        rhi::runtime::RenderPass _renderPass;
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;
        rhi::runtime::ImageMemoryReaderPtr _imageReader = nullptr;
    };

}