#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include "math/math.h"

#include <cstdint>
#include <span>
#include <vector>

namespace engine::graphics {

    struct CascadeFrameBuffer {
        rhi::runtime::ImagePtr image;
        std::vector<rhi::runtime::FrameBufferPtr> frameBuffers;
    };

    class CascadeShadowMap {
    public:
        CascadeShadowMap(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
            uint16_t size, uint8_t cascades, uint8_t bits, float near, float far);

        void createRenderData();
        void prepare();
        void initCascadeSplits() noexcept;
        void updateCascades(const mat4f & cameraInverse);

        void beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept;
        void endRenderPass(rhi::runtime::CommandList & commandList) const noexcept;

        const rhi::runtime::ImagePtr & getImage(uint8_t frameId) const noexcept;

        const vec3f & getDirection() const noexcept { return _direction; }
        bool setDirection(const vec3f & direction, float eps = 1e-5f) noexcept;

        std::span<const mat4f> getCascadeMatrices() const noexcept { return _cascadeMatrices; }
        rhi::runtime::RenderPass getRenderPass() const noexcept { return _renderPass; }

    private:
        std::vector<CascadeFrameBuffer> _framebuffers; // inflight frames count
        rhi::runtime::RenderPass _renderPass;
        rhi::SamplerValue _sampler;

        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;

        std::vector<mat4f> _cascadeMatrices;
        std::vector<float> _splitDepths;
        std::vector<float> _cascadeSplits;

		vec3f _direction = vec3f{0.0f, 0.0f, -1.0f};
        float _clipRange;
        float _near;
        float _far;
        float _cascadeSplitLambda = 0.95f;
		uint16_t _size;
        uint8_t _cascadesCount;
		uint8_t _bits;
    };

}