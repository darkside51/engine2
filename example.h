#pragma once

#include "file/file_manager.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/utils.h"
#include "graphics/render/inflight_buffer.h"
#include "threads/executor.h"

#include "input/input.h"

#include "graphics/ui/imgui.h"

#include <vector>

namespace example {

struct GBufferAttachments {
    engine::rhi::runtime::ImagePtr position, normal, albedo, emissive, emissive2, depth, metallicRoughness;
};

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

    void update(threads::ExecutionTimePoint now, float delta);
    void draw(threads::ExecutionContext & executionContext, threads::ExecutionTimePoint now, float delta);

    void deviceProcess();
    void setRenderThread(threads::ExecutionContext * renderThread) noexcept {
        _renderThread = renderThread;
    }

    void setMainThread(threads::ExecutionContext * mainThread) noexcept {
        _mainThread = mainThread;
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

    // ssao
    engine::rhi::runtime::BufferPtr _ssaoKernelBuffer;
    engine::rhi::runtime::ImagePtr _ssaoNoise;

    std::vector<GBufferAttachments> _gBufferAttachments;
    std::vector<engine::rhi::runtime::ImagePtr> _ssaoImages;
    std::vector<engine::rhi::runtime::ImagePtr> _ssaoBluredImages;
    std::vector<engine::rhi::runtime::FrameBufferPtr> _gBuffers;
    std::vector<engine::rhi::runtime::FrameBufferPtr> _ssaoBuffers;
    std::vector<engine::rhi::runtime::FrameBufferPtr> _emissiveBluredBuffers;

    engine::rhi::runtime::QueuePtr _renderQueue;
    std::vector<engine::rhi::runtime::Semaphore> _readyToPresentSemaphores;
    std::vector<engine::rhi::runtime::Semaphore> _transfersCompleteSemaphores;
    std::vector<engine::rhi::runtime::CommandListPtr> _commandLists;
    std::vector<engine::rhi::runtime::CommandListPtr> _transferCommandLists;
    std::vector<engine::rhi::runtime::CommandList*> _externalCommandLists;
    bool _readyToSubmitExternal = true;

    engine::rhi::FrameIndex _frameIndex;

    engine::ImGuiSystem _imgui;
    engine::rhi::runtime::BufferPtr _postPositions = nullptr;
    engine::rhi::runtime::BufferPtr _postIndices = nullptr;

    bool _canDraw = true;

    //queryPool
    engine::rhi::runtime::QueryPoolPtr _queryPool = nullptr;
    std::vector<std::vector<uint64_t>> _queryResults;

    //
    threads::ExecutionContext * _renderThread = nullptr;
    threads::ExecutionContext * _mainThread = nullptr;
};

}