#pragma once

#include "types.h"

namespace engine::rhi {

    struct GraphicsPipelineStatesInfo {
        struct ViewPortInfo {
            uint32_t viewportCount = 1u;
            uint32_t scissorCount = 1u;
        } viewportState;
    
        DynamicState dynamicStates;
    
        struct RasterisationInfo {
            PolygonMode polygonMode = PolygonMode::eFill;
            CullMode cullmode = CullMode::eNone; 
            FrontFace frontFace = FrontFace::eCounter_Clockwise;
        } rasterisationState;
    
        struct MultisampleStateInfo {
    
        } multisampleState;
    
        struct InputAssemblyStateInfo {
            PrimitiveTopology topology = PrimitiveTopology::eTriangles;
            bool restartEnabled = false;
        } inputAssemblyState;
    
        struct DepthStencilStateInfo {
            DepthState depth{};
            StencilState stencil{};
        } depthStencilState;
    
        struct TessellationStateInfo {
            uint32_t patchControlPoints = 0u;
        } tessellationState;
    
        std::vector<rhi::EncodedBlendMode> blendModes;
    
        struct VertexInputStateInfo {
            std::vector<rhi::VertexInputBinding> inputBindings;
            std::vector<rhi::VertexInputAttribute> attributes;
        } vertexInputState;
    };
    
}