#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/inflight_buffer.h"

#include <cstdint>
#include <memory>

namespace engine {

    class ImGuiRenderer {
    public:

    ImGuiRenderer();
    ~ImGuiRenderer();

    void createPipeline(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device, 
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass);

    void createFontTexture(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device);
    void createBuffers(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device);

    void beginFrame(float delta, uint16_t w, uint16_t h) noexcept;
    void endFrame();
    void prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device);
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept;

    private:
        rhi::runtime::PipelinePtr _pipeline = nullptr;
        rhi::runtime::BufferPtr _stagefontBuffer = nullptr;
        rhi::runtime::ImagePtr _fontImage = nullptr;
        rhi::SamplerInfo _sampler;
        std::unique_ptr<graphics::InflightBuffer> _vertices = nullptr;
        std::unique_ptr<graphics::InflightBuffer> _indices = nullptr;
        std::unique_ptr<graphics::InflightBuffer> _uniform = nullptr;
    };

}