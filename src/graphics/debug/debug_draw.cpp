#include "debug_draw.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include <array>

namespace engine::debug {
    
    struct Vertex {
        std::array<float, 3u> coord;
        uint32_t color;
    };
    
    constexpr uint32_t kVertexSize = sizeof(Vertex);

    void DebugDraw::destroy() noexcept {
        _linePipeline = nullptr;
        _fillPipeline = nullptr;
        _vertices = nullptr;
        _linesIndices = nullptr;
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
                .stride = kVertexSize,
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
        info.rasterisationState = {rhi::PolygonMode::eFill, rhi::CullMode::eBack};
        info.inputAssemblyState = {rhi::PrimitiveTopology::eTriangles, false};

        _fillPipeline = api->createGraphicsPipeline(device, renderPass, program, info, subpass);
    }

    void DebugDraw::createBuffers(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device) {

        constexpr uint32_t kInitialVerticesCountMax = 1024u;
        constexpr uint32_t kInitialIndicesCountMax = 1024u;

        const rhi::BufferInfo verticesInfo {
            rhi::BufferUsageFlags::eVertex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        const rhi::BufferInfo indicesInfo {
            rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        // vertices
        _vertices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, kVertexSize * kInitialVerticesCountMax
        );

        // indices
        _fillIndices = std::make_unique<graphics::InflightBuffer>(
            api, device, indicesInfo, sizeof(uint32_t) * kInitialIndicesCountMax
        );
        _linesIndices = std::make_unique<graphics::InflightBuffer>(
            api, device, indicesInfo, sizeof(uint32_t) * kInitialIndicesCountMax
        );
    }

    void DebugDraw::prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device) {

        // check we need to use a staging buffer
        const bool useStagingBuffer = !_vertices->isHostVisible() || !_fillIndices->isHostVisible() ||
                                      !_linesIndices->isHostVisible(); 

        const auto allSize = _vertices->sizeInBytes() + _fillIndices->sizeInBytes() + 
                             _linesIndices->sizeInBytes();

        if (useStagingBuffer) { 
            if (_staging == nullptr || allSize >= _staging->sizeInBytes()) {
                constexpr uint32_t kMinStagingSize = 100'000u /*100 Kb*/;
                _staging = std::make_unique<graphics::InflightStagingBuffer>(api, device, std::max(allSize * 2u, kMinStagingSize), 
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
            }

            _vertices->setStagingBuffer(_staging.get());

            _fillIndices->setStagingBuffer(_staging.get());
            _linesIndices->setStagingBuffer(_staging.get());

            _staging->map(frame);
        }

        _vertices->commit(commandList, frame);

        _fillIndices->commit(commandList, frame);
        _linesIndices->commit(commandList, frame);

        if (useStagingBuffer) {
            _staging->unmap();
        }
    }

    void DebugDraw::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept {

        if (_lineIndicesCount) { // draw lines
            commandList->bindPipeline(_linePipeline);

            commandList->bindVertexBuffer(0u, (*_vertices)(frame), 0u);
            commandList->bindIndexBuffer((*_linesIndices)(frame), 0u, rhi::IndexType::eU32);

            commandList->drawIndexed(static_cast<uint32_t>(_lineIndicesCount), 1u, 0u);

            _lineIndicesCount = 0u;
        }

        if (_filledIndicesCount) { // draw filled
            commandList->bindPipeline(_fillPipeline);

            commandList->bindVertexBuffer(0u, (*_vertices)(frame), 0u);
            commandList->bindIndexBuffer((*_fillIndices)(frame), 0u, rhi::IndexType::eU32);

            commandList->drawIndexed(static_cast<uint32_t>(_filledIndicesCount), 1u, 0u);

            _filledIndicesCount = 0u;
        }

        _verticesCount = 0u;
    }

    void DebugDraw::drawLines(std::span<const vec3f> linesVtxs, uint32_t color) {
        const auto vSize = static_cast<uint32_t>((_verticesCount + linesVtxs.size()) * kVertexSize);
        if (_vertices->sizeInBytes() <= vSize) {
            _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        }

        const auto lineParts = linesVtxs.size() - 1u;
        const auto newIndicesCount = lineParts * 2u;
        const auto iSize = static_cast<uint32_t>((_lineIndicesCount + newIndicesCount) * sizeof(uint32_t));
        if (_linesIndices->sizeInBytes() <= iSize) {
            _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
        }

        auto * vMem = _vertices->map2<Vertex>() + _verticesCount;
        auto * iMem = _linesIndices->map2<uint32_t>() + _lineIndicesCount;

        uint32_t i = 0u;
        for (const auto & v : linesVtxs) {
            *(vMem + i) = {
                .coord = {v.x, v.y, v.z},
                .color = color
            };

            ++i;
        }

        for (i = 0u; i < lineParts; ++i) {
            *(iMem + 2u * i) = _verticesCount + i;
            *(iMem + 2u * i + 1u) = _verticesCount + i + 1u;
        }

        _verticesCount += linesVtxs.size();
        _lineIndicesCount += newIndicesCount;

        _vertices->unmap();
        _linesIndices->unmap();
    }

    void DebugDraw::drawSquare(vec2f corner0, vec2f corner1, uint32_t color, Type t) {
        const auto vSize = static_cast<uint32_t>((_verticesCount + 4u) * kVertexSize);
        if (_vertices->sizeInBytes() <= vSize) {
            _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        }

        auto * vMem = _vertices->map2<Vertex>() + _verticesCount;

        *(vMem + 0u) = {
            .coord = {corner0.x, corner0.y, 0.0f},
            .color = color
        };

        *(vMem + 1u) = {
            .coord = {corner1.x, corner0.y, 0.0f},
            .color = color
        };

        *(vMem + 2u) = {
            .coord = {corner1.x, corner1.y, 0.0f},
            .color = color
        };

        *(vMem + 3u) = {
            .coord = {corner0.x, corner1.y, 0.0f},
            .color = color
        };

        if (std::to_underlying(t) & std::to_underlying(Type::eLines)) {
            const auto iSize = static_cast<uint32_t>((_lineIndicesCount + 8u) * sizeof(uint32_t));
            if (_linesIndices->sizeInBytes() <= iSize) {
                _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
            }

            auto * iMem = _linesIndices->map2<uint32_t>() + _lineIndicesCount;
            
            for (uint8_t i = 0u; i < 3u; ++i) {
                *(iMem + 2u * i) = _verticesCount + i;
                *(iMem + 2u * i + 1u) = _verticesCount + i + 1u;
            }

            *(iMem + 2u * 3u) = _verticesCount + 2u + 1u;
            *(iMem + 2u * 3u + 1u) = _verticesCount + 0u;

            _lineIndicesCount += 8u;
            _linesIndices->unmap();
        }

        if (std::to_underlying(t) & std::to_underlying(Type::eFill)) {
            const auto iSize2 = static_cast<uint32_t>((_filledIndicesCount + 6u) * sizeof(uint32_t));
            if (_fillIndices->sizeInBytes() <= iSize2) {
                _fillIndices->resize(std::max(_fillIndices->sizeInBytes() * 2u, iSize2));
            }

            auto * iMem2 = _fillIndices->map2<uint32_t>() + _filledIndicesCount;

            *(iMem2 + 0u) = _verticesCount + 0u;
            *(iMem2 + 1u) = _verticesCount + 1u;
            *(iMem2 + 2u) = _verticesCount + 2u;
            *(iMem2 + 3u) = _verticesCount + 2u;
            *(iMem2 + 4u) = _verticesCount + 3u;
            *(iMem2 + 5u) = _verticesCount + 0u;

            _filledIndicesCount += 6u;
            _fillIndices->unmap();
        }

        _verticesCount += 4u;
        _vertices->unmap();
    }

}