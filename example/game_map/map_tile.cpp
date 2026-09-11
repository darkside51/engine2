#include "map_tile.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/scene/scene.h"
#include "stl/vector.h"

namespace g_map {
    
    struct Vertex{
        engine::vec3f positions;
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
            stl::vector<Vertex> vertices(verticesInRow * verticesInRow);
            const float cellSize = static_cast<float>(_size) / _sizeInCells;

            for (size_t i = 0u; i < verticesInRow; ++i) {
                for (size_t j = 0u; j < verticesInRow; ++j) {
                    vertices[i * verticesInRow + j] = {
                        .positions = {j * cellSize, i * cellSize, 0.0f}
                    };
                }
            }

            cmd.updateBuffer(_vertices, 0u, vertices.size(), vertices.data());
        }
        {
            stl::vector<uint16_t> indices(_sizeInCells * _sizeInCells * 6u);

            for (size_t i = 0u; i < _sizeInCells; ++i) {
                for (size_t j = 0u; j < _sizeInCells; ++j) {
                    auto idx = i * _sizeInCells + j;
                    indices[idx + 0u] = idx;
                    indices[idx + 1u] = idx + 1u;
                    indices[idx + 2u] = idx + verticesInRow;

                    indices[idx + 3u] = idx + verticesInRow;
                    indices[idx + 4u] = idx + 1u;
                    indices[idx + 5u] = idx + verticesInRow + 1u;
                }
            }

            cmd.updateBuffer(_indices, 0u, indices.size(), indices.data());
        }
        
    }

    //
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
                    engine::rhi::runtime::Pipeline * pipeline = geometry.getPipeline();

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