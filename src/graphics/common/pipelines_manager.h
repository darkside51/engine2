#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "stl/vector.h"

#include <cstdint>

namespace engine::graphics {

    class PipelinesManager {
    public:
        PipelinesManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
        ~PipelinesManager() { destroy(); }

        void destroy();

        uint32_t registerGraphicsPipeline(rhi::runtime::PipelinePtr && pipeline);
        const rhi::runtime::PipelinePtr & getGraphicsPipeline(uint32_t pipelineId) const noexcept;

        uint32_t registerComputePipeline(rhi::runtime::PipelinePtr && pipeline);
        const rhi::runtime::PipelinePtr & getComputePipeline(uint32_t pipelineId) const noexcept;

    private:
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;

        stl::vector<rhi::runtime::PipelinePtr> _graphicsPipelines;
        stl::vector<rhi::runtime::PipelinePtr> _computesPipelines;

    };

}