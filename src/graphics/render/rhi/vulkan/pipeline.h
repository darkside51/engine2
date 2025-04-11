#pragma once

#include "graphics/render/rhi/common/types.h"
#include "vk_types.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <span>
#include <utility>

namespace engine::vulkan {

    VkPipelineViewportStateCreateInfo pipelineViewportStateInfo(uint32_t viewportCount = 1u, uint32_t scissorCount = 1u) noexcept;

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo(
        const std::pair<std::array<VkDynamicState, rhi::kDynamicStatesCount>, uint8_t> & statesArray) noexcept;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo() noexcept;

    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo(
        rhi::PolygonMode polygonMode, 
        rhi::CullMode cullmode, 
        rhi::FrontFace frontFace = rhi::FrontFace::eCounter_Clockwise) noexcept;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo(rhi::PrimitiveTopology topology,
        bool restartEnabled = false) noexcept;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo(const rhi::DepthState & depth, const rhi::StencilState & stencil) noexcept;

    template <size_t S>
    std::array<VkPipelineColorBlendAttachmentState, S> pipelineBlendAttachmentState(const std::array<uint32_t, S> & blendModes) noexcept {
        std::array<VkPipelineColorBlendAttachmentState, S> blendAttachmentState;
        size_t i = 0u;
        for (const auto blendValue : blendModes) {
            blendAttachmentState[i++] = utils::convert2vk(rhi::BlendMode{blendValue});
        }
        return blendAttachmentState;
    }

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
        std::span<const VkPipelineColorBlendAttachmentState> blendStates) noexcept;

    class Pipeline {
    public:


    // private:
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
    };

}
