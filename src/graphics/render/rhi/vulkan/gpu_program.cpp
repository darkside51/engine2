#include "gpu_program.h"

#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "vk_types.h"

#include <spirv_reflect.h>

#include <cassert>

namespace {
    engine::rhi::Format convert2Rhi(SpvReflectFormat format) noexcept {
        using namespace engine::rhi;
        switch (format) {
            case SPV_REFLECT_FORMAT_UNDEFINED: return Format::eUndefined;
            case SPV_REFLECT_FORMAT_R16_UINT: return Format::eR16_Uint;
            case SPV_REFLECT_FORMAT_R16_SINT: return Format::eR16_Sint;
            case SPV_REFLECT_FORMAT_R16_SFLOAT : return Format::eR16_Sfloat;
            case SPV_REFLECT_FORMAT_R16G16_UINT: return Format::eR16G16_Uint;
            case SPV_REFLECT_FORMAT_R16G16_SINT: return Format::eR16G16_Sint;
            case SPV_REFLECT_FORMAT_R16G16_SFLOAT: return Format::eR16G16_Sfloat;
            case SPV_REFLECT_FORMAT_R16G16B16_UINT: return Format::eR16G16B16_Uint;
            case SPV_REFLECT_FORMAT_R16G16B16_SINT: return Format::eR16G16B16_Sint;
            case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT: return Format::eR16G16B16_Sfloat;
            case SPV_REFLECT_FORMAT_R16G16B16A16_UINT: return Format::eR16G16B16A16_Uint;
            case SPV_REFLECT_FORMAT_R16G16B16A16_SINT: return Format::eR16G16B16A16_Sint;
            case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: return Format::eR16G16B16A16_Sfloat;
            case SPV_REFLECT_FORMAT_R32_UINT: return Format::eR32_Uint;
            case SPV_REFLECT_FORMAT_R32_SINT: return Format::eR32_Sint;
            case SPV_REFLECT_FORMAT_R32_SFLOAT: return Format::eR32_Sfloat;
            case SPV_REFLECT_FORMAT_R32G32_UINT: return Format::eR32G32_Uint;
            case SPV_REFLECT_FORMAT_R32G32_SINT: return Format::eR32G32_Sint;
            case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return Format::eR32G32_Sfloat;
            case SPV_REFLECT_FORMAT_R32G32B32_UINT: return Format::eR32G32B32_Uint;
            case SPV_REFLECT_FORMAT_R32G32B32_SINT: return Format::eR32G32B32_Sint;
            case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return Format::eR32G32B32_Sfloat;
            case SPV_REFLECT_FORMAT_R32G32B32A32_UINT: return Format::eR32G32B32A32_Uint;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SINT: return Format::eR32G32B32A32_Sint;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return Format::eR32G32B32A32_Sfloat;
            case SPV_REFLECT_FORMAT_R64_UINT: return Format::eR64_Uint;
            case SPV_REFLECT_FORMAT_R64_SINT: return Format::eR64_Sint;
            case SPV_REFLECT_FORMAT_R64_SFLOAT: return Format::eR64_Sfloat;
            case SPV_REFLECT_FORMAT_R64G64_UINT: return Format::eR64G64_Uint;
            case SPV_REFLECT_FORMAT_R64G64_SINT: return Format::eR64G64_Sint;
            case SPV_REFLECT_FORMAT_R64G64_SFLOAT: return Format::eR64G64_Sfloat;
            case SPV_REFLECT_FORMAT_R64G64B64_UINT: return Format::eR64G64B64_Uint;
            case SPV_REFLECT_FORMAT_R64G64B64_SINT: return Format::eR64G64B64_Sint;
            case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT: return Format::eR64G64B64_Sfloat;
            case SPV_REFLECT_FORMAT_R64G64B64A64_UINT: return Format::eR64G64B64A64_Uint;
            case SPV_REFLECT_FORMAT_R64G64B64A64_SINT: return Format::eR64G64B64A64_Sint;
            case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: return Format::eR64G64B64A64_Sfloat;
        }
    }

    engine::rhi::shaders::DescriptorType convert2Rhi(SpvReflectDescriptorType type) noexcept {
        using namespace engine::rhi::shaders;
        switch (type) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return DescriptorType::eSampler;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return DescriptorType::eCombined_Image_Sampler;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return DescriptorType::eSampled_Image;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return DescriptorType::eStorage_Image;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return DescriptorType::eUniform_Texel_Buffer;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return DescriptorType::eStorage_Texel_Buffer;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return DescriptorType::eUniform_Buffer;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return DescriptorType::eStorage_Buffer;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return DescriptorType::eUniform_Buffer_Dynamic;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return DescriptorType::eStorage_Buffer_Dynamic;
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return DescriptorType::eInput_Attachment;
            case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return DescriptorType::eAcceleretion_Structure;
        }
    }
}

namespace engine::vulkan {
    
    ModuleReflection::ModuleReflection(rhi::shaders::Stages stage, std::span<const std::byte> code) {
        SpvReflectShaderModule reflectedModule;
		SpvReflectResult result = spvReflectCreateShaderModule(code.size(), code.data(), &reflectedModule);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			assert(false);
		}

        // input attributes
        uint32_t attributesCount = 0u;
        spvReflectEnumerateInputVariables(&reflectedModule, &attributesCount, nullptr);
        std::vector<SpvReflectInterfaceVariable*> reflectAttributes(attributesCount);
        spvReflectEnumerateInputVariables(&reflectedModule, &attributesCount, reflectAttributes.data());

        in.reserve(attributesCount);
        for (const auto & attribute : reflectAttributes) {
            in.emplace_back(attribute->location, convert2Rhi(attribute->format));
        }

        // descriptor sets
		uint32_t setsCount = 0u;
		spvReflectEnumerateDescriptorSets(&reflectedModule, &setsCount, nullptr);
		std::vector<SpvReflectDescriptorSet*> reflectSets(setsCount);
		spvReflectEnumerateDescriptorSets(&reflectedModule, &setsCount, reflectSets.data());

        sets.reserve(setsCount);

        size_t s = 0u;
        for (const auto & set : reflectSets) {
            engine::dynamic_array<rhi::shaders::BindingLayout, rhi::shaders::BindingSetLayout::kMaxBindingsInSetOnStack> layouts;
            layouts.reserve(set->binding_count);
            const auto view = layouts.view();

            for (uint32_t i = 0u; i < set->binding_count; ++i) {
                const auto & bindingInfo = set->bindings[i];
                view[i] = {
                    .binding = static_cast<uint16_t>(bindingInfo->binding),
                    .type = convert2Rhi(bindingInfo->descriptor_type),
                    .shaderStages = stage,
                    .count = static_cast<uint16_t>(bindingInfo->count)
                };
            }

            sets.emplace_back(std::move(layouts));
        }

        // push constatns
        uint32_t pushConstantsCount = 0u;
        spvReflectEnumeratePushConstantBlocks(&reflectedModule, &pushConstantsCount, nullptr);
        std::vector<SpvReflectBlockVariable*> reflectPushConstants(pushConstantsCount);
        spvReflectEnumeratePushConstantBlocks(&reflectedModule, &pushConstantsCount, reflectPushConstants.data());

        pushConstants.reserve(pushConstantsCount);

        for (const auto * pushConstant : reflectPushConstants) {
            pushConstants.emplace_back(engine::vulkan::utils::convert2vk(stage), pushConstant->offset, pushConstant->size);
        }

        spvReflectDestroyShaderModule(&reflectedModule);
    }

    //
    ShaderModule::ShaderModule(const Device & device, rhi::shaders::Stages stage, std::span<const std::byte> code) : 
        _device(&device), 
        _reflection(stage, code) {
        const VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .codeSize = static_cast<uint32_t>(code.size()),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };

        if (vkCreateShaderModule(device(), &createInfo, nullptr, &_shaderModule) != VK_SUCCESS) {
            assert(false);
            return;
        }
    }

    ShaderModule::~ShaderModule() {
        if (_shaderModule != VK_NULL_HANDLE) {
            const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
            vkDestroyShaderModule(_device->operator()(), _shaderModule, allocationCallbacks);
        }
    }

}