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

        void createRenderPass(rhi::Format depthFormat, rhi::ImageAspectFlags depthAspectFlags);
        void createRenderPass2(rhi::Format depthFormat, rhi::ImageAspectFlags depthAspectFlags);

        void createRenderData();
        void createRenderData(stl::span<rhi::runtime::ImagePtr> depthImages);
        void createRenderData2(stl::span<rhi::runtime::ImagePtr> depthImages);
        
        void beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId, vec2u currentSize) noexcept;
        void beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId, vec2u currentSize, vec2i offset) noexcept;
        void endRenderPass(rhi::runtime::CommandList & commandList) noexcept;
        void prepareResults(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept;

        rhi::runtime::RenderPass getRenderPass() const noexcept { return _renderPass; }
        const rhi::runtime::ImagePtr & getImage(uint8_t frameId) const noexcept;
        rhi::runtime::ImagePtr & getImage(uint8_t frameId) noexcept;

        vec2u getSize() const noexcept { return _size; }

        template <typename T>
        T getValue(vec2u coords, uint8_t frameId) const noexcept {
            T value;
            _imageReader->getValue(coords.x, coords.y, frameId, &value, sizeof(value));
            return value;
        }

    private:
        stl::vector<ColorIdsFrameBuffer> _framebuffers; // inflight frames count
        stl::vector<rhi::runtime::ImagePtr> _externalDepthImages;
        rhi::runtime::RenderPass _renderPass = {};
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;
        rhi::runtime::ImageMemoryReaderPtr _imageReader = nullptr;
        rhi::ClearValue _clearColorValue;
        rhi::Format _format;
        vec2u _size;
        uint8_t _imagesCount;
    };

}