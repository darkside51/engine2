#include "pipeline.h"
#include "device.h"
#include "physical_device.h"
#include "instance.h"

#include "vk_types.h"

#include "log/log.h"

namespace engine::vulkan {

    VkPipelineViewportStateCreateInfo pipelineViewportStateInfo(uint32_t viewportCount, uint32_t scissorCount) noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
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

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo(
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

        VkPipelineTessellationStateCreateInfo pipelineTessellationStateInfo(uint32_t patchControlPoints) noexcept {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .patchControlPoints = patchControlPoints
            };
        }

        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateInfo(
            std::span<const VkVertexInputBindingDescription> inputBindings,
            std::span<const VkVertexInputAttributeDescription> attributes
        ) noexcept {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindings.size()),
                .pVertexBindingDescriptions = inputBindings.data(),
                .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
                .pVertexAttributeDescriptions = attributes.data()
            };
        }

        VkComputePipelineCreateInfo computePipelineCreateInfo(
            const GpuProgram & program,
            VkPipeline basePipelineHandle,
            int32_t basePipelineIndex
        ) noexcept {
            return {
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = program.stages()[0],
                .layout = program.pipelineLayout(),
                .basePipelineHandle = basePipelineHandle,
                .basePipelineIndex = basePipelineIndex
            };
        }

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
            uint32_t subpass,
            VkPipeline basePipelineHandle,
            int32_t basePipelineIndex
        ) noexcept {
            auto const stages = program.stages();
            return {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stageCount = static_cast<uint32_t>(stages.size()),
                .pStages = stages.data(),
                .pVertexInputState = vertexInputState,
                .pInputAssemblyState = inputAssemblyState,
                .pTessellationState = tessellationState,
                .pViewportState = viewportState,
                .pRasterizationState = rasterizationState,
                .pMultisampleState = multisampleState,
                .pDepthStencilState = depthStencilState,
                .pColorBlendState = colorBlendState,
                .pDynamicState = dynamicState,
                .layout = program.pipelineLayout(),
                .renderPass = renderPass,
                .subpass = subpass,
                .basePipelineHandle = basePipelineHandle,
                .basePipelineIndex = basePipelineIndex
            };
        }

        ///////
        GraphicsPipelineInfo & GraphicsPipelineInfo::viewportStateInfo(uint32_t viewportCount, uint32_t scissorCount) noexcept {
            _viewportState = pipelineViewportStateInfo(viewportCount, scissorCount);
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::dynamicStateInfo(rhi::DynamicState states) noexcept {
            _statesArray = utils::convert2vk(states);
            _dynamicState = pipelineDynamicStateInfo(_statesArray);
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::multisampleStateInfo() noexcept {
            _multisampleState = pipelineMultisampleStateInfo();
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::rasterizationStateInfo(
            rhi::PolygonMode polygonMode, 
            rhi::CullMode cullmode, 
            rhi::FrontFace frontFace) noexcept {
            _rasterizationState = pipelineRasterizationStateInfo(polygonMode, cullmode, frontFace);
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::inputAssemblyStateInfo(rhi::PrimitiveTopology topology,
            bool restartEnabled) noexcept {
            _inputAssemblyState = pipelineInputAssemblyStateInfo(topology, restartEnabled);
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::depthStencilStateInfo(const rhi::DepthState & depth, 
            const rhi::StencilState & stencil) noexcept {
            _depthStencilState = pipelineDepthStencilStateInfo(depth, stencil);
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::tessellationStateInfo(uint32_t patchControlPoints) noexcept {
            _tessellationState = pipelineTessellationStateInfo(patchControlPoints);
            return *this;
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::colorBlendStateCreateInfo(
            std::span<const rhi::EncodedBlendMode> blendModes) noexcept {
                _blendAttachmentState.reserve(blendModes.size());
                auto view = _blendAttachmentState.view();
                size_t i = 0u;
                for (const auto blendValue : blendModes) {
                    view[i++] = utils::convert2vk(rhi::BlendMode{blendValue});
                }
  
            _colorBlendState = pipelineColorBlendStateCreateInfo(view);

            return *this; 
        }

        GraphicsPipelineInfo & GraphicsPipelineInfo::vertexInputStateInfo(
            std::span<const rhi::VertexInputBinding> inputBindings,
            std::span<const rhi::VertexInputAttribute> attributes
            ) noexcept {
            _vertexBindings = utils::convert2vk(inputBindings);
            _vertexAttributes = utils::convert2vk(attributes);
            _vertexInputState = vulkan::pipelineVertexInputStateInfo(_vertexBindings.view(), _vertexAttributes.view());
            return *this;
        }

        GraphicsPipelineInfo::GraphicsPipelineInfo(const rhi::GraphicsPipelineStatesInfo & info) {
            viewportStateInfo(info.viewportState.viewportCount, info.viewportState.scissorCount);
            dynamicStateInfo(info.dynamicStates);
            rasterizationStateInfo(info.rasterisationState.polygonMode, info.rasterisationState.cullmode, info.rasterisationState.frontFace);
            multisampleStateInfo();
            inputAssemblyStateInfo(info.inputAssemblyState.topology, info.inputAssemblyState.restartEnabled);
            depthStencilStateInfo(info.depthStencilState.depth, info.depthStencilState.stencil);
            tessellationStateInfo(info.tessellationState.patchControlPoints);
            colorBlendStateCreateInfo(info.blendModes);
            vertexInputStateInfo(info.vertexInputState.inputBindings, info.vertexInputState.attributes);
        }

        VkGraphicsPipelineCreateInfo GraphicsPipelineInfo::create(
            const GpuProgram & program, 
            VkRenderPass renderPass,
            uint32_t subpass,
            VkPipeline basePipelineHandle,
            int32_t basePipelineIndex) const noexcept {
                return graphicsPipelineCreateInfo(
                    program,
                    &_vertexInputState,
                    &_inputAssemblyState,
                    &_tessellationState,
                    &_viewportState,
                    &_rasterizationState,
                    &_multisampleState,
                    &_depthStencilState,
                    &_colorBlendState,
                    &_dynamicState,
                    renderPass,
                    subpass,
                    basePipelineHandle,
                    basePipelineIndex
                );
            }

        ///////
        Pipeline::Pipeline(Device & device, const GpuProgram & program, const VkComputePipelineCreateInfo & info) : 
            layout(info.layout),
            bindPoint(VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE),
            _device(&device), 
            _program(&program)  {

            auto const allocationCallbacks = device.gpu().getInstance().getAllocationCallbacks();
            if (auto result = vkCreateComputePipelines(device(), device.pipelineCache(), 1, &info, allocationCallbacks, &pipeline); 
                result != VK_SUCCESS) {
                LOG_E("vulkan error : pipeline create no success!");
            }
        }

        Pipeline::Pipeline(Device & device, const GpuProgram & program, const VkGraphicsPipelineCreateInfo & info) :
            layout(info.layout),
            bindPoint(VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS),
            _device(&device), 
            _program(&program) {
            auto const allocationCallbacks = device.gpu().getInstance().getAllocationCallbacks();
            if (auto result = vkCreateGraphicsPipelines(device(), device.pipelineCache(), 1, &info, allocationCallbacks, &pipeline); 
                result != VK_SUCCESS) {
                LOG_E("vulkan error : pipeline create no success!");
                return;
            }
            LOG_I("vulkan: pipeline created");
        }

        Pipeline::~Pipeline() {
            if (_device && pipeline) {
                auto const allocationCallbacks = _device->gpu().getInstance().getAllocationCallbacks();
                vkDestroyPipeline((*_device)(), pipeline, allocationCallbacks);
                LOG_I("vulkan: pipeline destroyed");
            }
        }
}