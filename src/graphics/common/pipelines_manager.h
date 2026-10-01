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

    class PipelinesManager {
    public:
        PipelinesManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
        ~PipelinesManager() { destroy(); }

        void destroy();

        uint32_t registerGraphicsPipeline(rhi::runtime::PipelinePtr && pipeline);
        const rhi::runtime::PipelinePtr & getGraphicsPipeline(uint32_t pipelineId) const noexcept;

        uint32_t registerComputePipeline(rhi::runtime::PipelinePtr && pipeline);
        const rhi::runtime::PipelinePtr & getComputePipeline(uint32_t pipelineId) const noexcept;

        uint32_t createComputePipeline(const rhi::runtime::GpuProgramPtr & program);
        uint32_t createGraphicsPipeline(const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass,
            const rhi::GraphicsPipelineStatesInfo & info, uint32_t subpass = 0u);

        struct ShaderModuleDescriptor {
            stl::span<const std::byte> data;
            std::string_view name;
            rhi::shaders::Stages stage;
        };

        rhi::runtime::GpuProgramPtr & createProgram(
            stl::span<const ShaderModuleDescriptor> moduleDescriptors,
            rhi::shaders::SpecializationInfo && specializationInfo = {}
        );

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
    };

}