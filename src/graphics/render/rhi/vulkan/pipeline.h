#pragma once

#include "gpu_program.h"

#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/pipeline.h"
#include "graphics/render/rhi/common/runtime.h"
#include "vk_types.h"

#include "utils/dynamic_array.h"

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

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo(
        rhi::PolygonMode polygonMode, 
        rhi::CullMode cullmode, 
        rhi::FrontFace frontFace = rhi::FrontFace::eCounter_Clockwise) noexcept;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo(rhi::PrimitiveTopology topology,
        bool restartEnabled = false) noexcept;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo(const rhi::DepthState & depth, const rhi::StencilState & stencil) noexcept;

    template <size_t S>
    std::array<VkPipelineColorBlendAttachmentState, S> pipelineBlendAttachmentState(const std::array<rhi::EncodedBlendMode, S> & blendModes) noexcept {
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

    class GraphicsPipelineInfo {
    public:
        GraphicsPipelineInfo() = default;
        GraphicsPipelineInfo(const rhi::GraphicsPipelineStatesInfo & info);

        GraphicsPipelineInfo & viewportStateInfo(uint32_t viewportCount = 1u, uint32_t scissorCount = 1u) noexcept;
        GraphicsPipelineInfo & dynamicStateInfo(rhi::DynamicState states) noexcept;

        GraphicsPipelineInfo & multisampleStateInfo() noexcept;

        GraphicsPipelineInfo & rasterizationStateInfo(
            rhi::PolygonMode polygonMode, 
            rhi::CullMode cullmode, 
            rhi::FrontFace frontFace = rhi::FrontFace::eCounter_Clockwise) noexcept;

        GraphicsPipelineInfo & inputAssemblyStateInfo(rhi::PrimitiveTopology topology,
            bool restartEnabled = false) noexcept;

        GraphicsPipelineInfo & depthStencilStateInfo(const rhi::DepthState & depth, const rhi::StencilState & stencil) noexcept;

        GraphicsPipelineInfo & tessellationStateInfo(uint32_t patchControlPoints) noexcept;

        GraphicsPipelineInfo & colorBlendStateCreateInfo(
            std::span<const rhi::EncodedBlendMode> blendModes) noexcept;

        GraphicsPipelineInfo & vertexInputStateInfo(
            std::span<const rhi::VertexInputBinding> inputBindings,
            std::span<const rhi::VertexInputAttribute> attributes
        ) noexcept;

        VkGraphicsPipelineCreateInfo create(
            const GpuProgram & program, 
            VkRenderPass renderPass,
            uint32_t subpass = 0u,
            VkPipeline basePipelineHandle = VK_NULL_HANDLE,
            int32_t basePipelineIndex = 0) const noexcept;

    private:
        std::pair<std::array<VkDynamicState, rhi::kDynamicStatesCount>, uint8_t> _statesArray{};
        dynamic_array<VkVertexInputBindingDescription, 8u> _vertexBindings; // old value 4u
        dynamic_array<VkVertexInputAttributeDescription, 8u> _vertexAttributes;
        dynamic_array<VkPipelineColorBlendAttachmentState, 4u> _blendAttachmentState;

        VkPipelineVertexInputStateCreateInfo _vertexInputState{};
        VkPipelineInputAssemblyStateCreateInfo _inputAssemblyState{};
        VkPipelineTessellationStateCreateInfo _tessellationState{};
        VkPipelineViewportStateCreateInfo _viewportState{};
        VkPipelineRasterizationStateCreateInfo _rasterizationState{};
        VkPipelineMultisampleStateCreateInfo _multisampleState{};
        VkPipelineDepthStencilStateCreateInfo _depthStencilState{};
        VkPipelineColorBlendStateCreateInfo _colorBlendState{};
        VkPipelineDynamicStateCreateInfo _dynamicState{};
    };

    class Pipeline final : public rhi::runtime::Pipeline {
    public:
        Pipeline(Device & device, const GpuProgram & program, const VkComputePipelineCreateInfo & info);
        Pipeline(Device & device, const GpuProgram & program, const VkGraphicsPipelineCreateInfo & info);
        
        ~Pipeline();

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        // VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;

        const GpuProgram * getProgram() const noexcept { return _program; }
        const rhi::shaders::BindingSetLayout * getLayout(uint8_t setId) const noexcept;

    private:
        Device * _device = nullptr;
        const GpuProgram * _program = nullptr;
    };

}
