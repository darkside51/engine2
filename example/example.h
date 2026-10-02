#pragma once

#include "engine/src/file/file_manager.h"
#include "engine/src/graphics/render/rhi/common/runtime.h"
#include "engine/src/graphics/render/rhi/common/utils.h"
#include "engine/src/graphics/render/inflight_buffer.h"
#include "engine/src/threads/executor.h"

#include "engine/src/input/input.h"

#include "engine/src/graphics/ui/imgui.h"
#include "engine/src/stl/vector.h"

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

    // engine::FileManager _fileManager;
    engine::stl::vector<engine::rhi::runtime::GpuProgramPtr> _programs;
    engine::stl::vector<engine::rhi::runtime::PipelinePtr> _pipelines;

    std::unique_ptr<engine::rhi::runtime::Rhi> _api;
    engine::rhi::runtime::SurfacePtr _surface;

    engine::rhi::PhysicalDeviceFeatureRequest _gpuFeatures;
    engine::rhi::runtime::PysicalDevicePtr _gpu;
    engine::rhi::runtime::DevicePtr _device;

    engine::rhi::runtime::SwapchainPtr _swapchain;
    engine::stl::vector<engine::rhi::runtime::ImagePtr> _colorAttachmentImages;
    engine::stl::vector<engine::rhi::runtime::ImagePtr> _depthImages;

    engine::stl::vector<engine::rhi::runtime::FrameBufferPtr> _mainFrameBuffers;
    engine::stl::vector<engine::rhi::runtime::FrameBufferPtr> _sceneFrameBuffers;

    engine::stl::vector<GBufferAttachments> _gBufferAttachments;
    engine::stl::vector<engine::rhi::runtime::FrameBufferPtr> _gBuffers;
    engine::stl::vector<engine::rhi::runtime::FrameBufferPtr> _emissiveBluredBuffers;

    engine::rhi::runtime::QueuePtr _renderQueue;
    engine::stl::vector<engine::rhi::runtime::Semaphore> _readyToPresentSemaphores;
    engine::stl::vector<engine::rhi::runtime::Semaphore> _transfersCompleteSemaphores;
    engine::stl::vector<engine::rhi::runtime::CommandListPtr> _commandLists;
    engine::stl::vector<engine::rhi::runtime::CommandListPtr> _transferCommandLists;
    engine::stl::vector<engine::rhi::runtime::CommandList*> _externalCommandLists;
    bool _readyToSubmitExternal = true;

    engine::rhi::FrameIndex _frameIndex;

    engine::ImGuiSystem _imgui;
    engine::rhi::runtime::BufferPtr _postPositions = nullptr;
    engine::rhi::runtime::BufferPtr _postIndices = nullptr;

    bool _canDraw = true;

    //queryPool
    engine::rhi::runtime::QueryPoolPtr _queryPool = nullptr;
    engine::stl::vector<engine::stl::vector<uint64_t>> _queryResults;

    //
    threads::ExecutionContext * _renderThread = nullptr;
    threads::ExecutionContext * _mainThread = nullptr;
};

}