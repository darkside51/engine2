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
    enum class Type : uint8_t {
        eLines = 1u,
        eFill = 2u,
        eFull = eLines | eFill
    };

    // system
    void destroy() noexcept;

    void createPipelines(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device, 
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
        uint32_t subpass = 0u);

    void createBuffers(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device);

    void prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device);
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept;

    // user
    void drawSquare(vec2f corner0, vec2f corner1, uint32_t color, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);
    void drawCircle(vec2f center, float r, uint32_t color, uint8_t segments, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);
    void drawCube(vec3f corner0, vec3f corner1, uint32_t color, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);
    void drawSphere(vec3f center, float r, uint32_t color, uint8_t segments, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);

    void drawLines(std::span<const vec3f> linesVtxs, uint32_t color, const mat4f & transform = mat4f{1.0f});

private:
    rhi::runtime::PipelinePtr _linePipeline = nullptr;
    rhi::runtime::PipelinePtr _fillPipeline = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _vertices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _linesIndices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _fillIndices = nullptr;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

    uint32_t _verticesCount = 0u;
    uint32_t _filledIndicesCount = 0u;
    uint32_t _lineIndicesCount = 0u;
};

}