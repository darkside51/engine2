#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <span>
#include <vector>
#include <utility>

namespace engine::vulkan {

class Device;

struct ModuleReflection {
    struct InValue {
        uint32_t location = 0u;
        rhi::Format format = rhi::Format::eUndefined;
    };

    ModuleReflection(rhi::shaders::Stages stage, std::span<const std::byte> code);
    std::vector<InValue> in;
    std::vector<std::pair<rhi::shaders::BindingSetLayout, uint8_t>> sets; // pair of BindingSetLayout and set id
    std::vector<VkPushConstantRange> pushConstants;
    rhi::shaders::Stages stage = rhi::shaders::Stages::eNone;
};

class ShaderModule {
public:
    struct View {
        VkShaderModule shaderModule;
        const ModuleReflection * reflection;  
    };

    ShaderModule(const Device & device, rhi::shaders::Stages stage, std::span<const std::byte> code);
    ~ShaderModule();

    ShaderModule(const ShaderModule &) = delete;
    ShaderModule & operator= (const ShaderModule &) = delete;

    ShaderModule(ShaderModule && other) noexcept : 
        _device(other._device), 
        _shaderModule(other._shaderModule), 
        _reflection(std::move(other._reflection)) {
        other._shaderModule = VK_NULL_HANDLE;
    }

    ShaderModule & operator= (ShaderModule && other) noexcept {
        _device = other._device; 
        _shaderModule = other._shaderModule; 
        _reflection = std::move(other._reflection);
        other._shaderModule = VK_NULL_HANDLE;
        return *this;
    }

    inline explicit operator bool() const noexcept { return _shaderModule != VK_NULL_HANDLE; }
    VkShaderModule operator()() const noexcept { return _shaderModule; }

    View view() const noexcept {
        return {_shaderModule, &_reflection};
    }

private:
    const Device * _device = nullptr; 
    VkShaderModule _shaderModule = VK_NULL_HANDLE;
    ModuleReflection _reflection;
};

class GpuProgram {
public:
    GpuProgram(Device & device, std::span<const ShaderModule::View> moduleViews);
    ~GpuProgram();

    std::span<const VkPipelineShaderStageCreateInfo> stages() const noexcept {
        return _shaderStages;
    }

    const rhi::shaders::BindingSetLayout * getLayout(uint8_t setId) const noexcept;
    uint32_t setsCount() const noexcept { return _sets.size(); }
    std::span<const std::pair<rhi::shaders::BindingSetLayout, uint8_t>> setsView() const noexcept {
        return _sets;
    }

    uint64_t id() const noexcept { return _id; }

    VkPipelineLayout pipelineLayout() const noexcept { return _pipelineLayout; }
    inline explicit operator bool() const noexcept { return _pipelineLayout != VK_NULL_HANDLE; }

private:
    std::vector<std::pair<rhi::shaders::BindingSetLayout, uint8_t>> _sets;
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    Device * _device = nullptr; 
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    uint64_t _id = 0u;
};

}