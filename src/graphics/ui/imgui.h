#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/inflight_buffer.h"
#include "graphics/render/staging_buffer.h"

#include "input/input.h"

#include "math/math.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace engine {

    class ImGuiSystem final : public InputObserver {
    public:
        ImGuiSystem();
        ~ImGuiSystem();

        void destroy() noexcept;

        void createPipeline(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device, 
            const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
            uint32_t subpass = 0u);

        void createFontTexture(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device);
        void createBuffers(const std::unique_ptr<rhi::runtime::Rhi> & api,
            rhi::runtime::DevicePtr & device);

        void beginFrame(float delta, uint16_t w, uint16_t h) noexcept;
        void endFrame();
        void prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
            const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device);
        void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept;

        bool onInputPointerEvent(const PointerEvent & event) override;
        bool onInputWheelEvent(float dx, float dy) override;
        bool onInpuKeyEvent(const KeyEvent & event, const InputKeyMod & mod) override;
        bool onInpuCharEvent(uint16_t code) override { return false; }

    private:
        void setupKeyMap();
        rhi::runtime::PipelinePtr _pipeline = nullptr;
        rhi::runtime::BufferPtr _stagefontBuffer = nullptr;
        rhi::runtime::ImagePtr _fontImage = nullptr;
        rhi::SamplerInfo _sampler;
        std::unique_ptr<graphics::InflightBuffer> _vertices = nullptr;
        std::unique_ptr<graphics::InflightBuffer> _indices = nullptr;
        std::unique_ptr<graphics::InflightBuffer> _uniform = nullptr;
        std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

        std::unordered_map<uint8_t, std::pair<uint8_t, uint8_t>> _keyMap;

        vec4f _transform{};
    };

}