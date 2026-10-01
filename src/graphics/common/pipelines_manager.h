#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/runtime.h"
#include "stl/span.h"
#include "stl/vector.h"
#include "stl/unordered_map.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace engine::graphics {

    class RenderPassManager final {
        using PassesMap = stl::unordered_map<std::string, uint32_t, hash::String_hash, std::equal_to<>>;
        
    public:

    void destroy() noexcept {
        _passes.clear();
        _passesMap.clear();
    }

    private:
        stl::vector<rhi::runtime::RenderPass> _passes;
        PassesMap _passesMap;
    };

    class ShaderModulesManager final {
        using ModulesMap = stl::unordered_map<std::string, rhi::runtime::ShaderModulePtr, hash::String_hash, std::equal_to<>>;

    public:
        ShaderModulesManager(
            rhi::runtime::Rhi & api,
            rhi::runtime::Device & device
        ) : _api(api), _device(device) {}

        const rhi::runtime::ShaderModule & registerModule(std::string key, rhi::shaders::Stages stage, stl::span<const std::byte> code) {
            if (const auto * shaderModule = getModule(key)) {
                return *shaderModule;
            }

            return registerModule(std::move(key), _api.createShaderModule(_device, stage, code));
        }

        const rhi::runtime::ShaderModule & registerModule(std::string key, rhi::runtime::ShaderModulePtr && module) {
            return *(_modules.emplace(std::move(key), std::move(module)).first->second);
        }

        const rhi::runtime::ShaderModule * getModule(std::string_view key) const noexcept {
            if (auto it = _modules.find(key); it != _modules.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        void clear() noexcept {
            _modules.clear();
        }

    private:
        ModulesMap _modules;
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;
    };

    struct ShaderModuleDescriptor {
        stl::span<const std::byte> data;
        std::string_view name;
        rhi::shaders::Stages stage;
    };

    class PipelinesManager final {
        using ProgramsMap = stl::unordered_map<std::string, uint32_t, hash::String_hash, std::equal_to<>>;

    public:
        static constexpr uint32_t kNullPipelineID = 0xff'ff'ff'ffu;

        PipelinesManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
        ~PipelinesManager() { destroy(); }

        void destroy() noexcept;

        uint32_t registerGraphicsPipeline(rhi::runtime::PipelinePtr && pipeline);
        const rhi::runtime::PipelinePtr & getGraphicsPipeline(uint32_t pipelineId) const noexcept;

        uint32_t registerComputePipeline(rhi::runtime::PipelinePtr && pipeline);
        const rhi::runtime::PipelinePtr & getComputePipeline(uint32_t pipelineId) const noexcept;

        uint32_t createComputePipeline(const rhi::runtime::GpuProgramPtr & program);
        uint32_t createGraphicsPipeline(const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass,
            const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass = 0u);

        uint32_t createComputePipeline(uint32_t programIndex);
        uint32_t createGraphicsPipeline(uint32_t programIndex, rhi::runtime::RenderPass renderPass,
            const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass = 0u);

        uint32_t createComputePipeline(std::string_view programName);
        uint32_t createGraphicsPipeline(std::string_view programName, rhi::runtime::RenderPass renderPass,
            const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass = 0u);

        const rhi::runtime::GpuProgramPtr & createProgram(
            stl::span<const ShaderModuleDescriptor> moduleDescriptors,
            rhi::shaders::SpecializationInfo && specializationInfo = {}
        );

        const rhi::runtime::GpuProgramPtr & createProgram(
            std::string_view programName,
            stl::span<const ShaderModuleDescriptor> moduleDescriptors,
            rhi::shaders::SpecializationInfo && specializationInfo = {}
        );

        const rhi::runtime::GpuProgramPtr & getProgramByIndex(uint32_t idx) const noexcept;
        const rhi::runtime::GpuProgramPtr & getProgramByName(std::string_view name) const noexcept;

        ShaderModulesManager & getShaderModulesManager() noexcept {
            return _shaderModulesManager;
        }

        const ShaderModulesManager & getShaderModulesManager() const noexcept {
            return _shaderModulesManager;
        }

    private:
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;

        stl::vector<rhi::runtime::PipelinePtr> _graphicsPipelines;
        stl::vector<rhi::runtime::PipelinePtr> _computesPipelines;

        ShaderModulesManager _shaderModulesManager;
        stl::vector<rhi::runtime::GpuProgramPtr> _programs;
        ProgramsMap _programsMap;
    };

}