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
        info.rasterisationState = {rhi::PolygonMode::eFill, rhi::CullMode::eNone};
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
        _fillVertices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, kVertexSize * kInitialVerticesCountMax
        );
        _linesVertices = std::make_unique<graphics::InflightBuffer>(
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
        const bool useStagingBuffer = !_fillVertices->isHostVisible() || !_fillIndices->isHostVisible() ||
                                      !_linesVertices->isHostVisible() || !_linesIndices->isHostVisible(); 

        const auto allSize = _fillVertices->sizeInBytes() + _fillIndices->sizeInBytes() + 
                                _linesVertices->sizeInBytes() + _linesIndices->sizeInBytes();

        if (useStagingBuffer) { 
            if (_staging == nullptr || allSize >= _staging->sizeInBytes()) {
                constexpr uint32_t kMinStagingSize = 100'000u /*100 Kb*/;
                _staging = std::make_unique<graphics::InflightStagingBuffer>(api, device, std::max(allSize * 2u, kMinStagingSize), 
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
            }

            _fillVertices->setStagingBuffer(_staging.get());
            _fillIndices->setStagingBuffer(_staging.get());

            _linesVertices->setStagingBuffer(_staging.get());
            _linesIndices->setStagingBuffer(_staging.get());

            _staging->map(frame);
        }

        _fillVertices->commit(commandList, frame);
        _fillIndices->commit(commandList, frame);

        _linesVertices->commit(commandList, frame);
        _linesIndices->commit(commandList, frame);

        if (useStagingBuffer) {
            _staging->unmap();
        }
    }

    void DebugDraw::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept {

        if (_lineIndicesCount) { // draw lines
            commandList->bindPipeline(_linePipeline);

            commandList->bindVertexBuffer(0u, (*_linesVertices)(frame), 0u);
            commandList->bindIndexBuffer((*_linesIndices)(frame), 0u, rhi::IndexType::eU32);

            commandList->drawIndexed(static_cast<uint32_t>(_lineIndicesCount), 1u, 0u);

            _lineVerticesCount = 0u;
            _lineIndicesCount = 0u;
        }

        if (_filledIndicesCount) { // draw filled
            commandList->bindPipeline(_fillPipeline);

            commandList->bindVertexBuffer(0u, (*_fillVertices)(frame), 0u);
            commandList->bindIndexBuffer((*_fillIndices)(frame), 0u, rhi::IndexType::eU32);

            commandList->drawIndexed(static_cast<uint32_t>(_filledIndicesCount), 1u, 0u);

            _filledVerticesCount = 0u;
            _filledIndicesCount = 0u;
        }
    }

    void DebugDraw::drawLines(std::span<const vec3f> linesVtxs, uint32_t color) {
        const auto vSize = static_cast<uint32_t>((_lineVerticesCount + linesVtxs.size()) * kVertexSize);
        if (_linesVertices->sizeInBytes() <= vSize) {
            _linesVertices->resize(std::max(_linesVertices->sizeInBytes() * 2u, vSize));
        }

        const auto lineParts = linesVtxs.size() - 1u;
        const auto newIndicesCount = lineParts * 2u;
        const auto iSize = static_cast<uint32_t>((_lineIndicesCount + newIndicesCount) * sizeof(uint32_t));
        if (_linesIndices->sizeInBytes() <= iSize) {
            _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
        }

        auto * vMem = _linesVertices->map2<Vertex>() + _lineVerticesCount;
        auto * iMem = _linesIndices->map2<uint32_t>() + _lineIndicesCount;

        uint32_t i = 0u;
        for (const auto & v : linesVtxs) {
            *(vMem + i) = {
                .coord = {v.x, v.y, v.z},
                .color = color
            };

            *(iMem + 2u * i) = _lineVerticesCount + i;
            *(iMem + 2u * i + 1u) = _lineVerticesCount + i + 1u;

            ++i;
        }

        _lineVerticesCount += linesVtxs.size();
        _lineIndicesCount += newIndicesCount;

        _linesVertices->unmap();
        _linesIndices->unmap();
    }

}