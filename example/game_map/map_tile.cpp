#include "map_tile.h"

#include "graphics/common/graphics_service.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/scene/scene.h"
#include "graphics/texture/texture_manager.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>

namespace g_map {
    
    struct Vertex{
        engine::stl::array<float, 2u>(position);
    };

    TerrainTile::TerrainTile(uint16_t size, uint8_t sizeInCells): _size(size), _sizeInCells(sizeInCells) {}

    void TerrainTile::buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device) {
        using namespace engine;
        using namespace engine::rhi;

        const BufferInfo vBufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const BufferInfo iBufferInfo {
            BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const size_t verticesInRow = _sizeInCells + 1u;

        const ImageInfo normalsImageInfo{
            engine::rhi::ImageUsageFlags::eStorage | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR8G8B8A8_Snorm, {verticesInRow, verticesInRow, 1u}
        };

        const ImageInfo heightsImageInfo{
            engine::rhi::ImageUsageFlags::eStorage | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR16_Sfloat, {verticesInRow, verticesInRow, 1u}
        };

        const size_t vBufferSize = verticesInRow * verticesInRow * sizeof(Vertex);
        const size_t iBufferSize = _sizeInCells * _sizeInCells * 6u * sizeof(uint16_t); // all cells count * 6u
        _vertices = api.createBuffer(device, vBufferInfo, vBufferSize);
        _indices = api.createBuffer(device, iBufferInfo, iBufferSize);
        _normals = api.createImage(device, normalsImageInfo);
        _heights = api.createImage(device, heightsImageInfo);
    }

    void TerrainTile::buildGeometry(engine::rhi::runtime::CommandList & cmd) {
        using namespace engine;
        using namespace engine::rhi;

        const size_t verticesInRow = _sizeInCells + 1u;
        {
            stl::vector<std::byte> vertices(verticesInRow * verticesInRow * sizeof(Vertex));
            auto * verticeView = reinterpret_cast<Vertex*>(vertices.data());

            for (size_t i = 0u; i < verticesInRow; ++i) {
                for (size_t j = 0u; j < verticesInRow; ++j) {
                    verticeView[i * verticesInRow + j] = {
                        .position = {static_cast<float>(j), static_cast<float>(i)}
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
                    if (idx & 1) {
                        indicesView[n + 0u] = idx;
                        indicesView[n + 1u] = idx + 1u;
                        indicesView[n + 2u] = idx + verticesInRow + 1u;

                        indicesView[n + 3u] = idx;
                        indicesView[n + 4u] = idx + verticesInRow + 1u;
                        indicesView[n + 5u] = idx + verticesInRow;
                    } else {
                        indicesView[n + 0u] = idx;
                        indicesView[n + 1u] = idx + 1u;
                        indicesView[n + 2u] = idx + verticesInRow;

                        indicesView[n + 3u] = idx + verticesInRow;
                        indicesView[n + 4u] = idx + 1u;
                        indicesView[n + 5u] = idx + verticesInRow + 1u;
                    }

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
        _normals = nullptr;
        _heights = nullptr;
    }

    //
    void TerrainTileRenderer::dispatch(
        const engine::scene::CustomSceneObject & object, const engine::scene::Scene & scene, const engine::mat4f & worldTransform,
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
    ) const {
        const auto & geometry = static_cast<const TerrainTile &>(object.getImpl());
        engine::rhi::runtime::Pipeline * pipeline = geometry.getComputeHeightPipeline();
        assert(pipeline != nullptr);

        constexpr uint16_t kTileLocalGroupSize = 32u;
        const auto cellsCountInRow = geometry.getSizeInCells();
        const uint16_t verticesInRow = cellsCountInRow + 1u;

        const auto groupX = (verticesInRow + kTileLocalGroupSize - 1u) / kTileLocalGroupSize;
        const auto groupY = (verticesInRow + kTileLocalGroupSize - 1u) / kTileLocalGroupSize;

        const auto time = scene.getShadersTime();
        commandList->bindPipeline(*pipeline);
        commandList->pushConstants(engine::rhi::shaders::Stages::eCompute, 0u, sizeof(float), &time);
        commandList->bindStorageImage(0u, geometry.getHeightsImage(), std::nullopt, 0u);
        commandList->dispatch(groupX, groupY, 1u);

        pipeline = geometry.getComputeNormalsPipeline();
        assert(pipeline != nullptr);
        commandList->bindPipeline(*pipeline);
        commandList->bindStorageImage(0u, geometry.getNormalsImage(), std::nullopt, 0u);
        commandList->bindImage(1u, geometry.getHeightsImage(), scene.getLinearSampler(), 0u);
        commandList->dispatch(groupX, groupY, 1u);
    }

    void TerrainTileRenderer::draw(
            const engine::scene::CustomSceneObject & object, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, 
            engine::scene::RenderPass pass, const engine::mat4f & worldTransform, 
            engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
        ) const {
        struct Uniform {
            engine::mat4f worldTransform;
            float cellSize;
            float uvCellSize;
        };

        auto const sceneViewParamBufferOffset = scene.getCurrentViewParamsBufferOffset();
        const auto & geometry = static_cast<const TerrainTile&>(object.getImpl());
        const float cellSize = static_cast<float>(geometry.getSize()) / geometry.getSizeInCells();

        Uniform uniform {
            worldTransform,
            cellSize,
            0.0625f
        };

        switch (pass) {
            case engine::scene::RenderPass::eCommon:
            {
                const auto & gs = scene.getGraphicsService();
                const auto * pipeline = geometry.getGraphicsPipeline();

                if (commandList->bindPipeline(*pipeline)) {
                    commandList->pushConstants(
                        engine::rhi::shaders::Stages::eVertex | engine::rhi::shaders::Stages::eFragment, 
                        0u, 72u, &uniform
                    );
                    commandList->bindBuffer(0u, scene.getViewParamsUniformBuffer(frameId), 0u, sceneViewParamBufferOffset);
                }

                const auto & ib = geometry.getIndicesBuffer();

                commandList->bindStorageImage(1u, geometry.getHeightsImage(), std::nullopt, 0u);
                commandList->bindImage(2u, geometry.getNormalsImage(), scene.getLinearSampler(), 0u);
                commandList->bindImage(3u, gs.getTextureManager().getDefaultImage(), scene.getNearestSampler(), 0u);

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

    void TerrainTileRenderer::dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end) {
        for (auto it = begin; it != end; ++it) { // dirty!!!
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*it);
            dispatch(object, scene, object.node->getWorldTransform(), commandList, frame);
        }
    }

    void TerrainTileRenderer::draw(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, 
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        for (auto it = begin; it != end; ++it) { // dirty!!!
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*it);
            draw(object, scene, viewParams, pass, object.node->getWorldTransform(), commandList, frame);
        }
    }


    /////////////////////////
    constexpr size_t kVerticesInRow = 32u;
    constexpr size_t kCellsInRow = kVerticesInRow - 1u;

    Terrain::Terrain(TerrainSceneDelegate & sceneDelegate, engine::scene::Scene & scene, uint8_t w, uint8_t h) :
    _scene(scene),
    _width(w),
    _height(h),
    _tiles(w * h) {
        const auto terrainW = _width * kVerticesInRow;
        const auto terrainH = _height * kVerticesInRow;
        _terrainNode = scene.createNode();
        _terrainNode->setPivot({terrainW * 0.5f, terrainH * 0.5f, 0.0f}); // center of tile        
        _terrainNode->setBounds(
            engine::scene::Bounds::createCube(
                engine::vec3f{terrainW, terrainH, -static_cast<float>(kVerticesInRow) * 0.5f}, 
                engine::vec3f{0.0f, 0.0f, static_cast<float>(kVerticesInRow) * 0.5f}
            )
        );

        for (uint8_t i = 0u; i < _height; ++i) {
            for (uint8_t j = 0u; j < _width; ++j) {
                auto & tile = _tiles[i * _width + j];
                tile.imageIndex = 0u;
                tile.x = j;
                tile.y = i;
                
                auto * tileNode = scene.createNode();
                tileNode->setPosition({j * kVerticesInRow, i * kVerticesInRow, 0.0f});

                tileNode->setBounds(
                    engine::scene::Bounds::createCube(
                        engine::vec3f{kVerticesInRow, kVerticesInRow, -static_cast<float>(kVerticesInRow) * 0.5f}, 
                        engine::vec3f{0.0f, 0.0f, static_cast<float>(kVerticesInRow) * 0.5f}
                    )
                );

                _terrainNode->addChild(tileNode);

                auto * object = scene.createObject<engine::scene::CustomSceneObject>(sceneDelegate, tile);
                engine::scene::attachObjectToNode(*object, *tileNode);
            }   
        }
    }

    Terrain::~Terrain() {
        _scene.destroyNode(_terrainNode);
    }

    // scene delegate
    void TerrainSceneDelegate::buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device,
        uint16_t mapImageWidth, uint16_t mapImageHeight) {
        using namespace engine;
        using namespace engine::rhi;

        const BufferInfo vBufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const BufferInfo iBufferInfo {
            BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        // const size_t mapImageWidth = _width * kVerticesInRow;
        // const size_t mapImageHeight = _height * kVerticesInRow;

        const ImageInfo normalsImageInfo{
            engine::rhi::ImageUsageFlags::eStorage | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR8G8B8A8_Snorm, {mapImageWidth, mapImageHeight, 1u}
        };

        const ImageInfo heightsImageInfo{
            engine::rhi::ImageUsageFlags::eStorage | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR16_Sfloat, {mapImageWidth, mapImageHeight, 1u}
        };

        const size_t vBufferSize = kVerticesInRow * kVerticesInRow * sizeof(Vertex);
        const size_t iBufferSize = kCellsInRow * kCellsInRow * 6u * sizeof(uint16_t); // all cells count * 6u
        _tileCommonVertices = api.createBuffer(device, vBufferInfo, vBufferSize);
        _tileCommonIndices = api.createBuffer(device, iBufferInfo, iBufferSize);

        auto & terrainMaps0 = _terrainMaps[0u];

        terrainMaps0.normals = api.createImage(device, normalsImageInfo);
        terrainMaps0.heights = api.createImage(device, heightsImageInfo);
    }

    void TerrainSceneDelegate::buildGeometry(engine::rhi::runtime::CommandList & cmd) {
        using namespace engine;
        using namespace engine::rhi;

        {
            stl::vector<std::byte> vertices(kVerticesInRow * kVerticesInRow * sizeof(Vertex));
            auto * verticeView = reinterpret_cast<Vertex*>(vertices.data());

            for (size_t i = 0u; i < kVerticesInRow; ++i) {
                for (size_t j = 0u; j < kVerticesInRow; ++j) {
                    verticeView[i * kVerticesInRow + j] = {
                        .position = {static_cast<float>(j), static_cast<float>(i)}
                    };
                }
            }
            cmd.updateBuffer(_tileCommonVertices, 0u, std::move(vertices));
        }
        {
            stl::vector<std::byte> indices(kCellsInRow * kCellsInRow * 6u * sizeof(uint16_t));
            auto * indicesView = reinterpret_cast<uint16_t*>(indices.data());

            uint16_t n = 0u;
            for (size_t i = 0u; i < kCellsInRow; ++i) {
                for (size_t j = 0u; j < kCellsInRow; ++j) {
                    auto idx = i * kVerticesInRow + j;
                    if (idx & 1) {
                        indicesView[n + 0u] = idx;
                        indicesView[n + 1u] = idx + 1u;
                        indicesView[n + 2u] = idx + kVerticesInRow + 1u;

                        indicesView[n + 3u] = idx;
                        indicesView[n + 4u] = idx + kVerticesInRow + 1u;
                        indicesView[n + 5u] = idx + kVerticesInRow;
                    } else {
                        indicesView[n + 0u] = idx;
                        indicesView[n + 1u] = idx + 1u;
                        indicesView[n + 2u] = idx + kVerticesInRow;

                        indicesView[n + 3u] = idx + kVerticesInRow;
                        indicesView[n + 4u] = idx + 1u;
                        indicesView[n + 5u] = idx + kVerticesInRow + 1u;
                    }

                    n += 6u;
                }
            }
            cmd.updateBuffer(_tileCommonIndices, 0u, std::move(indices));
        }
    }

    bool TerrainSceneDelegate::compare(const engine::scene::CustomSceneObject & object0, 
        const engine::scene::CustomSceneObject & object1) const noexcept {
            const auto & tile0 = static_cast<const TerrainTile2 &>(object0.getImpl());
            const auto & tile1 = static_cast<const TerrainTile2 &>(object1.getImpl());
            return tile0.imageIndex < tile1.imageIndex;
    }

    void TerrainSceneDelegate::dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end) {

    }

    void TerrainSceneDelegate::draw(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, 
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame) {

    }

}