#pragma once

#include "file/file_manager.h"
#include "graphics/render/rhi/common/runtime.h"

#include <vector>

namespace example {

class Example {
public:
    Example();

    void createApi();
    bool createSurface(const engine::rhi::SurfaceCreateInfo & surfaceInfo);
    void createDevice();
    void createSwapchain();

    void resize(uint16_t w, uint16_t h);

private:
    engine::FileManager _fileManager;
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

    engine::rhi::runtime::QueuePtr _mainQueue;

};

}