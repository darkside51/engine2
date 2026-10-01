#include "pipelines_manager.h"

#include <string>

namespace engine::graphics {

    namespace {
        constexpr rhi::runtime::PipelinePtr kNullPipeline = nullptr;
    }
 
    PipelinesManager::PipelinesManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device) :
        _api(api),
        _device(device),
        _shaderModulesManager(api, device) {}

    void PipelinesManager::destroy() {
        _graphicsPipelines.clear();
        _computesPipelines.clear();
        _programs.clear();
        _shaderModulesManager.clear();
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

    rhi::runtime::GpuProgramPtr & PipelinesManager::createProgram(
            stl::span<const ShaderModuleDescriptor> moduleDescriptors,
            rhi::shaders::SpecializationInfo && specializationInfo
        ) {

        static stl::vector<rhi::runtime::ShaderModuleView> moduleViews;
        moduleViews.clear();

        for (const auto & m : moduleDescriptors) {
            moduleViews.emplace_back(_shaderModulesManager.registerModule(std::string{m.name}, m.stage, m.data).getView());
        }

        return _programs.emplace_back(_api.createProgram(_device, moduleViews));
    }

    uint32_t PipelinesManager::createComputePipeline(const rhi::runtime::GpuProgramPtr & program) {
        return registerComputePipeline(_api.createComputePipeline(_device, program));
    }

    uint32_t PipelinesManager::createGraphicsPipeline(const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
        const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass) {
        return registerGraphicsPipeline(_api.createGraphicsPipeline(_device, renderPass, program, info, subpass));
    }
}