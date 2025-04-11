#include "pipeline.h"

#include "vk_types.h"

namespace engine::vulkan {

    VkPipelineViewportStateCreateInfo pipelineViewportStateInfo(uint32_t viewportCount, uint32_t scissorCount) noexcept {
        return {
            .sType =VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = viewportCount,
            .pViewports = nullptr,
            .scissorCount = scissorCount,
            .pScissors = nullptr
        };
    }

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo(
        const std::pair<std::array<VkDynamicState, rhi::kDynamicStatesCount>, uint8_t> & statesArray) noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = statesArray.second,
            .pDynamicStates = statesArray.first.data()
        };
    }

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo() noexcept {
        // todo: setup this
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = false,
            .minSampleShading = 0.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable = false
        };
    }

    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo(
        rhi::PolygonMode polygonMode, 
        rhi::CullMode cullmode, 
        rhi::FrontFace frontFace
    ) noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = utils::convert2vk(polygonMode),
            .cullMode = utils::convert2vk(cullmode),
            .frontFace = utils::convert2vk(frontFace),
            .depthBiasEnable = false,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
        };
    }

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo(rhi::PrimitiveTopology topology, bool restartEnabled) noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = utils::convert2vk(topology),
            .primitiveRestartEnable = restartEnabled,
        };
    }

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo(const rhi::DepthState & depth, const rhi::StencilState & stencil) noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = depth.depthTestEnable,
            .depthWriteEnable = depth.depthWriteEnable,
            .depthCompareOp = utils::convert2vk(depth.depthCompareOp),
            .depthBoundsTestEnable = depth.depthBoundsTestEnable,
            .stencilTestEnable = stencil.stencilTestEnable,
            .front = utils::convert2vk(stencil.front),
            .back = utils::convert2vk(stencil.back),
            .minDepthBounds = depth.minDepthBounds,
            .maxDepthBounds = depth.maxDepthBounds
        };
    }

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
        std::span<const VkPipelineColorBlendAttachmentState> blendStates) noexcept {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .logicOpEnable = false,
                .logicOp = VK_LOGIC_OP_CLEAR,
                .attachmentCount = static_cast<uint32_t>(blendStates.size()),
                .pAttachments = blendStates.data(),
                .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
            };
        }
}