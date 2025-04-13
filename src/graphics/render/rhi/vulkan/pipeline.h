#pragma once

#include "gpu_program.h"

#include "graphics/render/rhi/common/types.h"
#include "vk_types.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <span>
#include <utility>

namespace engine::vulkan {

    class Device;

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

    VkPipelineTessellationStateCreateInfo pipelineTessellationStateInfo(uint32_t patchControlPoints) noexcept;

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateInfo(
        std::span<const VkVertexInputBindingDescription> inputBindings,
        std::span<const VkVertexInputAttributeDescription> attributes
    ) noexcept;

    VkComputePipelineCreateInfo computePipelineCreateInfo(
        const GpuProgram & program,
        VkPipeline basePipelineHandle = VK_NULL_HANDLE,
        int32_t basePipelineIndex = 0
    ) noexcept;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
        const GpuProgram & program, 
        const VkPipelineVertexInputStateCreateInfo * vertexInputState,
        const VkPipelineInputAssemblyStateCreateInfo * inputAssemblyState,
        const VkPipelineTessellationStateCreateInfo * tessellationState,
        const VkPipelineViewportStateCreateInfo * viewportState,
        const VkPipelineRasterizationStateCreateInfo * rasterizationState,
        const VkPipelineMultisampleStateCreateInfo * multisampleState,
        const VkPipelineDepthStencilStateCreateInfo * depthStencilState,
        const VkPipelineColorBlendStateCreateInfo * colorBlendState,
        const VkPipelineDynamicStateCreateInfo * dynamicState,
        VkRenderPass renderPass,
        uint32_t subpass = 0u,
        VkPipeline basePipelineHandle = VK_NULL_HANDLE,
        int32_t basePipelineIndex = 0
    ) noexcept;

    // typedef struct VkRayTracingPipelineCreateInfoKHR {
    //     VkStructureType                                      sType;
    //     const void*                                          pNext;
    //     VkPipelineCreateFlags                                flags;
    //     uint32_t                                             stageCount;
    //     const VkPipelineShaderStageCreateInfo*               pStages;
    //     uint32_t                                             groupCount;
    //     const VkRayTracingShaderGroupCreateInfoKHR*          pGroups;
    //     uint32_t                                             maxPipelineRayRecursionDepth;
    //     const VkPipelineLibraryCreateInfoKHR*                pLibraryInfo;
    //     const VkRayTracingPipelineInterfaceCreateInfoKHR*    pLibraryInterface;
    //     const VkPipelineDynamicStateCreateInfo*              pDynamicState;
    //     VkPipelineLayout                                     layout;
    //     VkPipeline                                           basePipelineHandle;
    //     int32_t                                              basePipelineIndex;
    // } VkRayTracingPipelineCreateInfoKHR;

    // typedef struct VkRayTracingPipelineCreateInfoNV {
    //     VkStructureType                               sType;
    //     const void*                                   pNext;
    //     VkPipelineCreateFlags                         flags;
    //     uint32_t                                      stageCount;
    //     const VkPipelineShaderStageCreateInfo*        pStages;
    //     uint32_t                                      groupCount;
    //     const VkRayTracingShaderGroupCreateInfoNV*    pGroups;
    //     uint32_t                                      maxRecursionDepth;
    //     VkPipelineLayout                              layout;
    //     VkPipeline                                    basePipelineHandle;
    //     int32_t                                       basePipelineIndex;
    // } VkRayTracingPipelineCreateInfoNV;

    class Pipeline {
    public:
        Pipeline(Device & device, const VkComputePipelineCreateInfo & info);
        Pipeline(Device & device, const VkGraphicsPipelineCreateInfo & info);
        
        ~Pipeline();

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;

    private:
        Device * _device = nullptr;
    };

}
