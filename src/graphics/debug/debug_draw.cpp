#include "debug_draw.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"
#include "math/math.h"

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

    void DebugDraw::drawLines(std::span<const vec3f> linesVtxs, uint32_t color, const mat4f & transform) {
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
            const auto p = transform * vec4f{v, 1.0f};
            *(vMem + i) = {
                .coord = {p.x / p.w, p.y / p.w, p.z / p.w},
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

    void DebugDraw::drawSquare(vec2f corner0, vec2f corner1, uint32_t color, const mat4f & transform, Type t) {
        const auto vSize = static_cast<uint32_t>((_verticesCount + 4u) * kVertexSize);
        if (_vertices->sizeInBytes() <= vSize) {
            _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        }

        auto * vMem = _vertices->map2<Vertex>() + _verticesCount;

        auto vp = transform * vec4f{corner0, 0.0f, 1.0f};
        *(vMem + 0u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner1.x, corner0.y, 0.0f, 1.0f};
        *(vMem + 1u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner1, 0.0f, 1.0f};
        *(vMem + 2u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner0.x, corner1.y, 0.0f, 1.0f};
        *(vMem + 3u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
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

    void DebugDraw::drawCircle(vec2f center, float r, uint32_t color, uint8_t segments, const mat4f & transform, Type t) {
        const auto vSize = static_cast<uint32_t>((_verticesCount + segments) * kVertexSize);
        if (_vertices->sizeInBytes() <= vSize) {
            _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        }

        auto * vMem = _vertices->map2<Vertex>() + _verticesCount;

        float angle = 0.0f;
        const float step = 2.0f * pi / segments;

        for (uint8_t s = 0u; s < segments; ++s) {
            const vec2f p = center + r * vec2f{std::cos(angle), std::sin(angle)};
            const auto vp = transform * vec4f{p, 0.0f, 1.0f};
            *(vMem + s) = {
                .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
                .color = color
            };
            angle += step;
        }

        if (std::to_underlying(t) & std::to_underlying(Type::eLines)) {
            const auto iSize = static_cast<uint32_t>((_lineIndicesCount + segments * 2u) * sizeof(uint32_t));
            if (_linesIndices->sizeInBytes() <= iSize) {
                _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
            }

            auto * iMem = _linesIndices->map2<uint32_t>() + _lineIndicesCount;

            for (uint8_t i = 0u; i < segments - 1u; ++i) {
                *(iMem + 2u * i) = _verticesCount + i;
                *(iMem + 2u * i + 1u) = _verticesCount + i + 1u;
            }

            *(iMem + 2u * (segments - 1u)) = _verticesCount + segments - 1u;
            *(iMem + 2u * (segments - 1u) + 1u) = _verticesCount + 0u;

            _lineIndicesCount += segments * 2u;
            _linesIndices->unmap();
        }

        if (std::to_underlying(t) & std::to_underlying(Type::eFill)) {
            const auto indexesCount = (segments - 2u) * 3u;
            const auto iSize2 = static_cast<uint32_t>((_filledIndicesCount + indexesCount) * sizeof(uint32_t));
            if (_fillIndices->sizeInBytes() <= iSize2) {
                _fillIndices->resize(std::max(_fillIndices->sizeInBytes() * 2u, iSize2));
            }

            auto * iMem2 = _fillIndices->map2<uint32_t>() + _filledIndicesCount;
            auto centralIndex = (segments / 2u);

            uint8_t j = 0u;
            for (uint8_t i = 0u; i < segments - 2u; ++i, ++j) {
                if (centralIndex > 0u && i >= centralIndex - 1u) {
                    centralIndex =  0u;
                    ++j;
                }

                *(iMem2 + 3u * i) = _verticesCount + j;
                *(iMem2 + 3u * i + 1u) = _verticesCount + j + 1u;
                *(iMem2 + 3u * i + 2u) = _verticesCount + centralIndex;
            }

            _filledIndicesCount += indexesCount;
            _fillIndices->unmap();
        }

        _verticesCount += segments;
        _vertices->unmap();
    }

    void DebugDraw::drawSphere(vec3f center, float r, uint32_t color, uint8_t segments, const mat4f & transform, Type t) {
        const auto vSize = static_cast<uint32_t>((_verticesCount + segments * segments) * kVertexSize);
        if (_vertices->sizeInBytes() <= vSize) {
            _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        }

        auto * vMem = _vertices->map2<Vertex>() + _verticesCount;

        float angle = 0.0f;
        const float step = 2.0f * pi / segments;

        for (uint8_t i = 0u; i < segments; ++i) {
            const int j = 2 * (i - segments / 2);
            const auto sin = std::sin(j * pi / segments / 2.0f);
            const auto cos = std::cos(j * pi / segments / 2.0f);
            for (uint8_t s = 0u; s < segments; ++s) {
                vec3f p = center + r * cos * vec3f{std::cos(angle), std::sin(angle), 0.0f};
                p.z += r * sin;
                const auto vp = transform * vec4f{p, 1.0f};
                *(vMem + s + (i * segments)) = {
                    .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
                    .color = color
                };
                angle += step;
            }
        }

        if (std::to_underlying(t) & std::to_underlying(Type::eLines)) {
            const auto iSize = static_cast<uint32_t>((_lineIndicesCount + segments * 2u * segments + segments * 2u * segments) * sizeof(uint32_t));
            if (_linesIndices->sizeInBytes() <= iSize) {
                _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
            }

            auto * iMem = _linesIndices->map2<uint32_t>() + _lineIndicesCount;

            {
                for (uint8_t j = 0u; j < segments; ++j) {
                    const auto vertexStart = _verticesCount + segments * j;
                    auto * mem = iMem + j * 2u * segments;
                    for (uint8_t i = 0u; i < segments - 1u; ++i) {
                        *(mem + 2u * i) = vertexStart + i;
                        *(mem + 2u * i + 1u) = vertexStart + i + 1u;
                    }

                    *(mem + 2u * (segments - 1u)) = vertexStart + segments - 1u;
                    *(mem + 2u * (segments - 1u) + 1u) = vertexStart + 0u;
                }
            }

            {
                for (uint8_t j = 0u; j < segments; ++j) {
                    auto * mem = iMem + segments * 2u * segments + j * segments * 2u;
                    const auto vertexStart = _verticesCount + j;
                    for (uint8_t i = 0u; i < segments - 1u; ++i) {
                        *(mem + 2u * i) = vertexStart + i * segments;
                        *(mem + 2u * i + 1u) = vertexStart + (i + 1u) * segments;
                    }

                    *(mem + 2u * (segments - 1u)) = vertexStart + (segments - 1u) * segments;
                    *(mem + 2u * (segments - 1u) + 1u) = _verticesCount + segments - 1u - j + (segments - 1u) * segments;
                }
            }

            _lineIndicesCount += segments * 2u * segments + segments * 2u * segments;
            _linesIndices->unmap();
        }

        _verticesCount += segments * segments;
        _vertices->unmap();
    }

    void DebugDraw::drawCube(vec3f corner0, vec3f corner1, uint32_t color, const mat4f & transform, Type t) {
        const auto vSize = static_cast<uint32_t>((_verticesCount + 4u) * kVertexSize);
        if (_vertices->sizeInBytes() <= vSize) {
            _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        }

        auto * vMem = _vertices->map2<Vertex>() + _verticesCount;

        auto vp = transform * vec4f{corner0, 1.0f};
        *(vMem + 0u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner1.x, corner0.y, corner0.z, 1.0f};
        *(vMem + 1u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner1.x, corner1.y, corner0.z, 1.0f};
        *(vMem + 2u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner0.x, corner1.y, corner0.z, 1.0f};
        *(vMem + 3u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner0.x, corner0.y, corner1.z, 1.0f};
        *(vMem + 4u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner1.x, corner0.y, corner1.z, 1.0f};
        *(vMem + 5u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner1, 1.0f};
        *(vMem + 6u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        vp = transform * vec4f{corner0.x, corner1.y, corner1.z, 1.0f};
        *(vMem + 7u) = {
            .coord = {vp.x / vp.w, vp.y / vp.w, vp.z / vp.w},
            .color = color
        };

        if (std::to_underlying(t) & std::to_underlying(Type::eLines)) {
            const auto iSize = static_cast<uint32_t>((_lineIndicesCount + 24u) * sizeof(uint32_t));
            if (_linesIndices->sizeInBytes() <= iSize) {
                _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
            }

            auto * iMem = _linesIndices->map2<uint32_t>() + _lineIndicesCount;
            for (uint8_t j = 0u; j < 2u; ++j) {
                auto * mem = iMem + 8u * j;
                const auto vertexStart = _verticesCount + j * 4u;
                for (uint8_t i = 0u; i < 3u; ++i) {
                    *(mem + 2u * i) = vertexStart + i;
                    *(mem + 2u * i + 1u) = vertexStart + i + 1u;
                }

                *(mem + 2u * 3u) = vertexStart + 2u + 1u;
                *(mem + 2u * 3u + 1u) = vertexStart + 0u;
            }

            *(iMem + 16u) = _verticesCount + 0u;
            *(iMem + 17u) = _verticesCount + 4u;
            *(iMem + 18u) = _verticesCount + 1u;
            *(iMem + 19u) = _verticesCount + 5u;
            *(iMem + 20u) = _verticesCount + 2u;
            *(iMem + 21u) = _verticesCount + 6u;
            *(iMem + 22u) = _verticesCount + 3u;
            *(iMem + 23u) = _verticesCount + 7u;

            _lineIndicesCount += 24u;
            _linesIndices->unmap();
        }


        _verticesCount += 4u;
        _vertices->unmap();
    }

}