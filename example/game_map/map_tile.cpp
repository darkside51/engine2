#include "map_tile.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/scene/scene.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>

namespace g_map {
    
    struct Vertex{
        engine::stl::array<float, 4u>(position);
    };

    TerrainTile::TerrainTile(uint16_t size, uint8_t sizeInCells): _size(size), _sizeInCells(sizeInCells) {}

    void TerrainTile::buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device) {
        using namespace engine;
        using namespace engine::rhi;

        const BufferInfo vBufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const engine::rhi::BufferInfo iBufferInfo {
            BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const size_t verticesInRow = _sizeInCells + 1u;
        const size_t vBufferSize = verticesInRow * verticesInRow * sizeof(Vertex);
        const size_t iBufferSize = _sizeInCells * _sizeInCells * 6u * sizeof(uint16_t); // all cells count * 6u
        _vertices = api.createBuffer(device, vBufferInfo, vBufferSize);
        _indices = api.createBuffer(device, iBufferInfo, iBufferSize);
    }

    void TerrainTile::buildGeometry(engine::rhi::runtime::CommandList & cmd) {
        using namespace engine;
        using namespace engine::rhi;

        const size_t verticesInRow = _sizeInCells + 1u;
        {
            stl::vector<std::byte> vertices(verticesInRow * verticesInRow * sizeof(Vertex));
            auto * verticeView = reinterpret_cast<Vertex*>(vertices.data());
            const float cellSize = static_cast<float>(_size) / _sizeInCells;

            for (size_t i = 0u; i < verticesInRow; ++i) {
                for (size_t j = 0u; j < verticesInRow; ++j) {
                    verticeView[i * verticesInRow + j] = {
                        .position = {j * cellSize, i * cellSize, 0.0f, 1.0f}
                    };
                }
            }
            cmd.updateBuffer(_vertices, 0u, std::move(vertices));
        }
        {
            stl::vector<std::byte> indices(_sizeInCells * _sizeInCells * 6u * sizeof(uint16_t));
            auto * indicesView = reinterpret_cast<uint16_t*>(indices.data());

            uint16_t n = 0u;
            for (size_t i = 0u; i < _sizeInCells; ++i) {
                for (size_t j = 0u; j < _sizeInCells; ++j) {
                    auto idx = i * verticesInRow + j;
                    indicesView[n + 0u] = idx;
                    indicesView[n + 1u] = idx + 1u;
                    indicesView[n + 2u] = idx + verticesInRow;

                    indicesView[n + 3u] = idx + verticesInRow;
                    indicesView[n + 4u] = idx + 1u;
                    indicesView[n + 5u] = idx + verticesInRow + 1u;

                    n += 6u;
                }
            }
            cmd.updateBuffer(_indices, 0u, std::move(indices));
        }
    }

    void TerrainTile::destroy() noexcept {
        _pipeline = nullptr;
        _vertices = nullptr;
        _indices = nullptr;
    }

    //
    void TerrainTileRenderer::dispatch(
        const engine::scene::CustomDrawObject & object, const engine::scene::Scene & scene, const engine::mat4f & worldTransform,
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
    ) const {
        const auto & geometry = *static_cast<TerrainTile*>(object.getData());
        engine::rhi::runtime::Pipeline * pipeline = geometry.getComputePipeline();
        assert(pipeline != nullptr);

        constexpr uint16_t kTileLocalGroupSize = 32u;
        const auto cellsCountInRow = geometry.getSizeInCells();
        const uint16_t verticesInRow = cellsCountInRow + 1u;

        const auto localExecutionCount = (verticesInRow % kTileLocalGroupSize == 0u) ?
                                          verticesInRow / kTileLocalGroupSize :
                                          verticesInRow / kTileLocalGroupSize + 1u;

        const auto time = scene.getShadersTime();
        commandList->bindPipeline(*pipeline);
        commandList->pushConstants(engine::rhi::shaders::Stages::eCompute, 0u, sizeof(float), &time);
        commandList->bindBuffer(0u, geometry.getVerticesBuffer(), 0u);
        commandList->dispatch(localExecutionCount, localExecutionCount, 1u);
    }

    void TerrainTileRenderer::draw(
            const engine::scene::CustomDrawObject & object, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, 
            engine::scene::RenderPass pass, const engine::mat4f & worldTransform, 
            engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
        ) const {
            struct Uniform {
                engine::mat4f worldTransform;
            };

            Uniform uniform {
                worldTransform
            };

            auto const sceneViewParamBufferOffset = scene.getCurrentViewParamsBufferOffset();
            const auto & geometry = *static_cast<TerrainTile*>(object.getData());
            switch (pass) {
                case engine::scene::RenderPass::eCommon:
                {
                    engine::rhi::runtime::Pipeline * pipeline = geometry.getGraphicsPipeline();

                    if (commandList->bindPipeline(*pipeline)) {
                        commandList->pushConstants(
                            engine::rhi::shaders::Stages::eVertex, 
                            0u, sizeof(Uniform), &uniform
                        );
                        commandList->bindBuffer(0u, scene.getViewParamsUniformBuffer(frameId), 0u, sceneViewParamBufferOffset);
                    }

                    const auto & ib = geometry.getIndicesBuffer();

                    commandList->bindVertexBuffer(0u, geometry.getVerticesBuffer());
                    commandList->bindIndexBuffer(ib, 0u, engine::rhi::IndexType::eU16);
                    commandList->drawIndexed(ib->sizeInBytes() / sizeof(uint16_t));
                }
                break;
                case engine::scene::RenderPass::eShadow:
                break;
                default: 
                break;
            }
        }

}