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
    std::vector<rhi::shaders::BindingSetLayout> sets;
    std::vector<VkPushConstantRange> pushConstants;
};

class ShaderModule {
public:
    ShaderModule(const Device & device, rhi::shaders::Stages stage, std::span<const std::byte> code);
    ~ShaderModule();

    inline explicit operator bool() const noexcept { return _shaderModule != VK_NULL_HANDLE; }
    VkShaderModule operator()() const noexcept { return _shaderModule; }

private:
    const Device * _device = nullptr; 
    ModuleReflection _reflection;
    VkShaderModule _shaderModule = VK_NULL_HANDLE;
};

class GpuProgram {
public:

private:    
};

}