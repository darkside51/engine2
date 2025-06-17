#pragma once

#include "file/file_manager.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/utils.h"

#include "graphics/ui/imgui.h"

#include <vector>

namespace example {

class Example {
public:
    Example();

    void destroy();

    void createApi();
    bool createSurface(const engine::rhi::SurfaceCreateInfo & surfaceInfo);
    void createDevice();
    void createSwapchain();

    void resize(uint16_t w, uint16_t h);

    void createRenderPasses();
    void createPipelines();

    void prepare();

    void draw(float delta);

private:
    void loadShaders();

    engine::FileManager _fileManager;
    engine::rhi::runtime::ShaderModulesManagerPtr _shaderModulesManager;

    std::vector<engine::rhi::runtime::GpuProgramPtr> _programs;
    std::unique_ptr<engine::rhi::runtime::Rhi> _api;
    engine::rhi::runtime::SurfacePtr _surface;

    engine::rhi::PhysicalDeviceFeatureRequest _gpuFeatures;
    engine::rhi::runtime::PysicalDevicePtr _gpu;
    engine::rhi::runtime::DevicePtr _device;

    engine::rhi::runtime::SwapchainPtr _swapchain;
    std::vector<engine::rhi::runtime::ImagePtr> _depthImages;
    std::vector<engine::rhi::runtime::RenderPass> _renderPasses;
    std::vector<engine::rhi::runtime::FrameBufferPtr> _mainFrameBuffers;

    engine::rhi::runtime::QueuePtr _renderQueue;
    std::vector<engine::rhi::runtime::Semaphore> _readyToPresentSemaphores;
    std::vector<engine::rhi::runtime::CommandListPtr> _commandLists;

    engine::rhi::FrameIndex _frameIndex;

    engine::ImGuiSystem _imgui;

    bool _canDraw = true;
};

}