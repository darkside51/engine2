#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "math/math.h"
#include "stl/span.h"
#include "stl/vector.h"

#include <cstdint>

namespace engine::graphics {
    
    struct ColorIdsFrameBuffer {
        rhi::runtime::ImagePtr image = nullptr;
        rhi::runtime::ImagePtr depthImage = nullptr;
        rhi::runtime::FrameBufferPtr frameBuffer = {};
    };

    class ColorPicker {
    public:
        ColorPicker(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
            uint8_t imagesCount, vec2u size, 
            rhi::ClearValue clearColorValue, 
            rhi::Format format = rhi::Format::eR32_Uint);

        void createRenderData();
        void createRenderData(stl::span<rhi::runtime::ImagePtr> depthImages);
        void createRenderData(stl::span<rhi::runtime::ImagePtr> depthImages, vec2i offset);
        
        void beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId, vec2u currentSize) noexcept;
        void endRenderPass(rhi::runtime::CommandList & commandList) noexcept;
        void prepareResults(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept;

        const rhi::runtime::ImagePtr & getImage(uint8_t frameId) const noexcept {
            return _framebuffers[frameId].image;
        }

        vec2u getSize() const noexcept { return _size; }
        uint32_t getValue(vec2u coords) const noexcept;

    private:
        stl::vector<ColorIdsFrameBuffer> _framebuffers; // inflight frames count
        stl::vector<rhi::runtime::ImagePtr> _externalDepthImages;
        rhi::runtime::RenderPass _renderPass;
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;
        rhi::runtime::ImageMemoryReaderPtr _imageReader = nullptr;
        rhi::ClearValue _clearColorValue;
        rhi::Format _format;
        vec2u _size;
        vec2i _frameBufferOffset = {0, 0};
        uint8_t _imagesCount;
    };

}