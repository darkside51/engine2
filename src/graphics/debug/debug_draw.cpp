#include "debug_draw.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

namespace engine::debug {
    
    void DebugDraw::destroy() noexcept {
        _linePipeline = nullptr;
        _fillPipeline = nullptr;
        _linesVertices = nullptr;
        _linesIndices = nullptr;
        _fillVertices = nullptr;
        _fillIndices = nullptr;
        _staging = nullptr;
    }

    void DebugDraw::createPipelines(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device, 
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass, 
        uint32_t subpass) {

        rhi::GraphicsPipelineStatesInfo info;
        info.viewportState = {1u, 1u};
        info.blendModes = {rhi::common_blend_modes::blend_none};
        info.depthStencilState = {
            {
                .depthTestEnable = true,
                .depthWriteEnable = false,
                .depthCompareOp = rhi::CompareOp::eLess
            }, 
            {}
        };
        info.rasterisationState = {rhi::PolygonMode::eLine, rhi::CullMode::eNone};
        info.inputAssemblyState = {rhi::PrimitiveTopology::eLines, false};

        info.vertexInputState = {
            std::vector{rhi::VertexInputBinding{
                .stride = 3u * sizeof(float) + sizeof(uint32_t),
                .binding = 0u,
                .inputRate = rhi::VertexInputRate::eVertex
            }},
            std::vector{
                rhi::VertexInputAttribute{ // position
                    .location = 0u,
                    .binding = 0u,
                    .format = rhi::Format::eR32G32B32_Sfloat,
                    .offset = 0u
                },
                rhi::VertexInputAttribute{ // color
                    .location = 1u,
                    .binding = 0u,
                    .format = rhi::Format::eR8G8B8A8_Unorm,
                    .offset = 3u * sizeof(float)
                }
            }
        };

        _linePipeline = api->createGraphicsPipeline(device, renderPass, program, info, subpass);

        info.blendModes = {rhi::common_blend_modes::blend_alpha};
        info.rasterisationState = {rhi::PolygonMode::eFill, rhi::CullMode::eNone};
        info.inputAssemblyState = {rhi::PrimitiveTopology::eTriangles, false};

        _fillPipeline = api->createGraphicsPipeline(device, renderPass, program, info, subpass);
    }

    void DebugDraw::prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device) {

    }

}