#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/inflight_buffer.h"
#include "graphics/render/staging_buffer.h"
#include "math/math.h"

#include <cstdint>
#include <chrono>
#include <span>

namespace engine::scene {
    struct Bounds;
}

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

    void createPipelines(rhi::runtime::Rhi & api,
        rhi::runtime::Device & device,
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
        uint32_t subpass = 0u, bool reverseDepth = true);

    void createBuffers(rhi::runtime::Rhi & api,
        rhi::runtime::Device & device);

    void prepare(rhi::runtime::CommandList & commandList, uint8_t frame);
    void draw(rhi::runtime::CommandList & commandList, uint8_t frame, std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

    // user
    void drawSquare(vec2f corner0, vec2f corner1, uint32_t color, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);
    void drawCircle(vec2f center, float r, uint32_t color, uint8_t segments, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);
    void drawCube(vec3f corner0, vec3f corner1, uint32_t color, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);
    void drawSphere(vec3f center, float r, uint32_t color, uint8_t segments, const mat4f & transform = mat4f{1.0f}, Type t = Type::eFull);

    void drawLines(std::span<const vec3f> linesVtxs, uint32_t color, const mat4f & transform = mat4f{1.0f});

    void drawBounds(const scene::Bounds & bounds, uint32_t color, const mat4f & transform = mat4f{1.0f});
    void drawGizmo(float size, const mat4f & transform = mat4f{1.0f});

private:
    uint32_t calculateTargetStagingSize() const noexcept;
    void reduceUsedMemory(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

    rhi::runtime::Rhi * _api = nullptr;
    rhi::runtime::Device * _device = nullptr;

    rhi::runtime::PipelinePtr _linePipeline = nullptr;
    rhi::runtime::PipelinePtr _fillPipeline = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _vertices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _linesIndices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _fillIndices = nullptr;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

    uint32_t _verticesCount = 0u;
    uint32_t _filledIndicesCount = 0u;
    uint32_t _lineIndicesCount = 0u;

    std::chrono::time_point<std::chrono::steady_clock> _tryReduceUsedMemoryTime;
    uint32_t _lastFramesUsedVertexCount = 0u;
    uint32_t _lastFramesUsedFilledIndicesCount = 0u;
    uint32_t _lastFramesUsedLineIndicesCount = 0u;
};

}