#include "gpu_program.h"

#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "vk_types.h"

#include "common/hash.h"

#include <spirv_reflect.h>

#include <algorithm>
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
    
    ModuleReflection::ModuleReflection(rhi::shaders::Stages stage, std::span<const std::byte> code) : 
        stage(stage) {
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

            sets.emplace_back(std::move(layouts), set->set);
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

    ///////////////////////////
    GpuProgram::GpuProgram(Device & device, std::span<const ShaderModule::View> moduleViews) : _device(&device) {
        uint32_t setsCount = 0u;
        _shaderStages.reserve(moduleViews.size());

        size_t hash = 0u;
        for (const auto & view : moduleViews) {
            setsCount += view.reflection->sets.size();
            _shaderStages.push_back(VkPipelineShaderStageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0u, // VkPipelineShaderStageCreateFlagBits - todo!
                    .stage = static_cast<VkShaderStageFlagBits>(utils::convert2vk(view.reflection->stage)),
                    .module = view.shaderModule,
                    .pName = "main",
                    .pSpecializationInfo = nullptr // todo!
                }
            );

            hash::hash_combo(hash, view.shaderModule);
            // if (pSpecializationInfo) { // todo!
            //     hash::hash_combo(hash, pSpecializationInfo);
            // }
        }

        _id = static_cast<uint64_t>(hash);

        _sets.reserve(setsCount);
        
        for (const auto & view : moduleViews) {
            for (const auto & set : view.reflection->sets) {
                auto it = std::find_if(_sets.begin(), _sets.end(), [setId = set.second](const auto & set){
                    return set.second == setId;
                });

                if (it == _sets.end()) {
                    _sets.emplace_back(set);
                } else {
                    const auto & layoutsView = set.first.layouts.view();
                    for (auto & l : layoutsView) {
                        if (auto * layout = it->first.findLayout(l.binding)) {
                            layout->shaderStages = layout->shaderStages | view.reflection->stage;
                        } else {
                            it->first.layouts.emplace_back(l);
                        }
                    }
                }
            }
        }

        _sets.shrink_to_fit();

        std::sort(_sets.begin(), _sets.end(), [](const auto & s0, const auto & s1) noexcept {
            return s0.second < s1.second;
        });

        // create pipeline layout
        std::vector<VkDescriptorSetLayout> vkSetLayouts; // vector?
        vkSetLayouts.reserve(_sets.size());
        auto & descriptorSetLayoutManager = _device->getDescriptorSetLayoutManager();
        for (const auto & [layout, _] : _sets) {
            vkSetLayouts.push_back(descriptorSetLayoutManager.getSetlayout(layout));
        }

        auto const pipelineLayoutCreateInfo = VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0, // VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT 
                        // specifies that implementations must ensure that the properties and/or absence 
                        // of a particular descriptor set do not influence any other properties of the pipeline layout. 
                        // This allows pipelines libraries linked without VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT 
                        // to be created with a subset of the total descriptor sets.
            .setLayoutCount =  static_cast<uint32_t>(vkSetLayouts.size()),
            .pSetLayouts = vkSetLayouts.data(),
            .pushConstantRangeCount = 0u, // todo!
            .pPushConstantRanges = nullptr           
        };

        const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
        auto const result = vkCreatePipelineLayout(_device->operator()(), &pipelineLayoutCreateInfo, 
                                                   allocationCallbacks, &_pipelineLayout); // maybe need to cache it?
        if (result != VK_SUCCESS) {
            assert(false);
        }
    }

    GpuProgram::~GpuProgram() {
        if (_pipelineLayout) {
            const auto * allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
            vkDestroyPipelineLayout(_device->operator()(), _pipelineLayout, allocationCallbacks);
            _pipelineLayout = VK_NULL_HANDLE;
        }
    }

    const rhi::shaders::BindingSetLayout * GpuProgram::getLayout(uint8_t setId) const noexcept {
        if (auto it = std::find_if(_sets.begin(), _sets.end(), [setId](const auto & set) {
            return set.second == setId;
        }); it != _sets.end()) {
            return &it->first;
        }

        return nullptr;
    }

}