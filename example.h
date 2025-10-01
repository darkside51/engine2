#pragma once

#include "file/file_manager.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/utils.h"
#include "graphics/render/inflight_buffer.h"

#include "input/input.h"

#include "graphics/ui/imgui.h"

#include <vector>

namespace threads {
    class ExecutionContext;
}

namespace example {

class Example : public engine::InputHandler {
public:
    Example();
    ~Example();

    void destroy();

    void createApi();
    bool createSurface(const engine::rhi::SurfaceCreateInfo & surfaceInfo);
    void createDevice();
    void createSwapchain();

    void resize(uint16_t w, uint16_t h);
    void sizeChanged();

    void createRenderPasses();
    void createPipelines();

    void prepare();

    void update(float delta);
    void draw(float delta);

    void deviceProcess();
    void setRenderThread(threads::ExecutionContext * renderThread) noexcept {
        _renderThread = renderThread;
    }

private:
    void loadShaders();
    void createDefaultResources();

    void prepareFrameDrawData(engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, float delta);

    engine::FileManager _fileManager;
    engine::rhi::runtime::ShaderModulesManagerPtr _shaderModulesManager;

    std::vector<engine::rhi::runtime::GpuProgramPtr> _programs;
    std::vector<engine::rhi::runtime::PipelinePtr> _pipelines;

    std::unique_ptr<engine::rhi::runtime::Rhi> _api;
    engine::rhi::runtime::SurfacePtr _surface;

    engine::rhi::PhysicalDeviceFeatureRequest _gpuFeatures;
    engine::rhi::runtime::PysicalDevicePtr _gpu;
    engine::rhi::runtime::DevicePtr _device;

    engine::rhi::runtime::SwapchainPtr _swapchain;
    std::vector<engine::rhi::runtime::ImagePtr> _colorAttachmentImages;
    std::vector<engine::rhi::runtime::ImagePtr> _depthImages;
    std::vector<engine::rhi::runtime::RenderPass> _renderPasses;
    std::vector<engine::rhi::runtime::FrameBufferPtr> _mainFrameBuffers;
    std::vector<engine::rhi::runtime::FrameBufferPtr> _sceneFrameBuffers;

    engine::rhi::runtime::QueuePtr _renderQueue;
    std::vector<engine::rhi::runtime::Semaphore> _readyToPresentSemaphores;
    std::vector<engine::rhi::runtime::Semaphore> _transfersCompleteSemaphores;
    std::vector<engine::rhi::runtime::CommandListPtr> _commandLists;
    std::vector<engine::rhi::runtime::CommandListPtr> _transferCommandLists;

    engine::rhi::FrameIndex _frameIndex;

    engine::ImGuiSystem _imgui;
    engine::rhi::runtime::BufferPtr _postPositions = nullptr;
    engine::rhi::runtime::BufferPtr _postIndices = nullptr;
    engine::rhi::runtime::ImagePtr _defaultImage = nullptr;
    std::unique_ptr<engine::graphics::InflightBuffer> _uniformBufferI = nullptr;
    engine::rhi::runtime::BufferPtr _storageBuffer = nullptr;
    engine::rhi::runtime::BufferPtr _cpuStorageResult = nullptr;
    bool _canDraw = true;

    // sphere
    engine::rhi::runtime::BufferPtr _sphereVtx = nullptr;
    engine::rhi::runtime::BufferPtr _sphereIdx = nullptr;
    engine::rhi::runtime::ImagePtr _sphereImage = nullptr;

    //queryPool
    engine::rhi::runtime::QueryPoolPtr _queryPool = nullptr;
    std::vector<std::vector<uint64_t>> _queryResults;

    //
    threads::ExecutionContext * _renderThread = nullptr;
};

}