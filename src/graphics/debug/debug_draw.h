#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/inflight_buffer.h"
#include "graphics/render/staging_buffer.h"
#include "math/math.h"

#include <cstdint>
#include <span>

namespace engine::debug {

class DebugDraw {
public:
    // system
    void destroy() noexcept;

    void createPipelines(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device, 
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
        uint32_t subpass = 0u);

    void prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device);
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept;

    // user
    void drawCircle(vec2f center, float r);
    void drawBox(vec2f corner0, vec2f corner1);
    void drawSphere(vec3f center, float r);
    void drawCube(vec3f corner0, vec3f corner1);
    void drawLines(std::span<const vec3f> linesVtxs);

private:
    rhi::runtime::PipelinePtr _linePipeline = nullptr;
    rhi::runtime::PipelinePtr _fillPipeline = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _vertices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _indices = nullptr;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;
};

}