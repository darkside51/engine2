#include "pipelines_manager.h"

namespace engine::graphics {

    namespace {
        constexpr rhi::runtime::PipelinePtr kNullPipeline = nullptr;
    }
 
    PipelinesManager::PipelinesManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device) :
        _api(api),
        _device(device) {}

    void PipelinesManager::destroy() {
        _graphicsPipelines.clear();
        _computesPipelines.clear();
    }

    uint32_t PipelinesManager::registerGraphicsPipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) { [[likely]]
            _graphicsPipelines.emplace_back(std::move(pipeline));
            return _graphicsPipelines.size() - 1u;
        }

        return 0xff'ff'ff'ffu;
    }

    const rhi::runtime::PipelinePtr & PipelinesManager::getGraphicsPipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _graphicsPipelines.size()) { [[unlikely]]
            return kNullPipeline;
        }
        return _graphicsPipelines[pipelineId];
    }

    uint32_t PipelinesManager::registerComputePipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) { [[likely]]
            _computesPipelines.push_back(std::move(pipeline));
            return _computesPipelines.size() - 1u;
        }

        return 0xff'ff'ff'ffu;
    }

    const rhi::runtime::PipelinePtr & PipelinesManager::getComputePipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _computesPipelines.size()) { [[unlikely]]
            return kNullPipeline;
        }
        return _computesPipelines[pipelineId];
    }
}