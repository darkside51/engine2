#include "pipelines_manager.h"
#include "render_pass_manager.h"
#include "log/log.h"

namespace engine::graphics {

    namespace {
        constexpr rhi::runtime::PipelinePtr kNullPipeline = nullptr;
        constexpr rhi::runtime::GpuProgramPtr kNullProgram = nullptr;
    }
 
    PipelinesManager::PipelinesManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device) :
        _api(api),
        _device(device),
        _shaderModulesManager(api, device) {}

    void PipelinesManager::destroy() noexcept {
        _graphicsPipelines.clear();
        _computesPipelines.clear();
        _programs.clear();
        _programsMap.clear();
        _shaderModulesManager.clear();
    }

    uint32_t PipelinesManager::registerGraphicsPipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) { [[likely]]
            _graphicsPipelines.emplace_back(std::move(pipeline));
            return _graphicsPipelines.size() - 1u;
        }

        LOG_E("PipelinesManager::registerGraphicsPipeline: invalid pipeline");
        return kNullPipelineID;
    }

    const rhi::runtime::PipelinePtr & PipelinesManager::getGraphicsPipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _graphicsPipelines.size()) { [[unlikely]]
            LOG_E("PipelinesManager::getGraphicsPipeline: no pipeline with id %s", pipelineId);
            return kNullPipeline;
        }
        return _graphicsPipelines[pipelineId];
    }

    uint32_t PipelinesManager::registerComputePipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) { [[likely]]
            _computesPipelines.push_back(std::move(pipeline));
            return _computesPipelines.size() - 1u;
        }

        LOG_E("PipelinesManager::registerComputePipeline: invalid pipeline");
        return kNullPipelineID;
    }

    const rhi::runtime::PipelinePtr & PipelinesManager::getComputePipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _computesPipelines.size()) { [[unlikely]]
            LOG_E("PipelinesManager::getComputePipeline: no pipeline with id %s", pipelineId);
            return kNullPipeline;
        }
        return _computesPipelines[pipelineId];
    }

    const rhi::runtime::GpuProgramPtr & PipelinesManager::createProgram(
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

    const rhi::runtime::GpuProgramPtr & PipelinesManager::createProgram(
            std::string_view programName,
            stl::span<const ShaderModuleDescriptor> moduleDescriptors,
            rhi::shaders::SpecializationInfo && specializationInfo
    ) {
        if (auto & p = getProgram(programName); p != kNullProgram) {
            LOG_D("PipelinesManager::createProgram: program with name %s, already exists", programName.data());
            return p;
        }

        _programsMap.emplace(programName, _programs.size());
        return createProgram(moduleDescriptors, std::move(specializationInfo));
    }

    const rhi::runtime::GpuProgramPtr & PipelinesManager::getProgram(uint32_t idx) const noexcept {
        assert(_programs.size() >= idx);
        return _programs[idx];
    }

    const rhi::runtime::GpuProgramPtr & PipelinesManager::getProgram(std::string_view name) const noexcept {
        if (auto it = _programsMap.find(name); it != _programsMap.end()) {
            return getProgram(it->second);
        }

        LOG_E("PipelinesManager::getProgramByName: no program with name %s", name.data());
        return kNullProgram;
    }

    uint32_t PipelinesManager::createComputePipeline(const rhi::runtime::GpuProgramPtr & program) {
        return registerComputePipeline(_api.createComputePipeline(_device, program));
    }

    uint32_t PipelinesManager::createGraphicsPipeline(const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
        const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass) {
        return registerGraphicsPipeline(_api.createGraphicsPipeline(_device, renderPass, program, info, subpass));
    }

    uint32_t PipelinesManager::createComputePipeline(uint32_t programIndex) {
        return createComputePipeline(getProgram(programIndex));
    }

    uint32_t PipelinesManager::createGraphicsPipeline(uint32_t programIndex, rhi::runtime::RenderPass renderPass,
        const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass) {
        return createGraphicsPipeline(getProgram(programIndex), renderPass, info, subpass);
    }

    uint32_t PipelinesManager::createComputePipeline(std::string_view programName) {
        if (const auto & p = getProgram(programName); p != kNullProgram) {
            return createComputePipeline(p);
        }
        return kNullPipelineID;
    }

    uint32_t PipelinesManager::createGraphicsPipeline(std::string_view programName, rhi::runtime::RenderPass renderPass,
        const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass) {
        if (const auto & p = getProgram(programName); p != kNullProgram) {
            return createGraphicsPipeline(p, renderPass, info, subpass);
        }
        return kNullPipelineID;
    }
}