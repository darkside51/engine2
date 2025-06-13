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
    bool createDevice();

private:
    engine::FileManager _fileManager;
    std::vector<engine::rhi::runtime::GpuProgramPtr> _programs;
    std::unique_ptr<engine::rhi::runtime::Rhi> _api;
    engine::rhi::runtime::SurfacePtr _surface;
    engine::rhi::runtime::PysicalDevicePtr _gpu;
};

}