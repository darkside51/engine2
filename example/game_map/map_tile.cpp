#include "map_tile.h"

#include "common/align.h"
#include "graphics/common/graphics_service.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/scene/scene.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/utils/vertex_processing.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>
#include <limits>

namespace g_map {
    constexpr uint16_t kMaxTileIds = 4096u;
    constexpr size_t kFloat16Size = 2u;

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

        const auto groupX = engine::div_ceil(verticesInRow, kTileLocalGroupSize);
        const auto groupY = engine::div_ceil(verticesInRow, kTileLocalGroupSize);

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
    constexpr uint32_t kCellsInRow = 31u;
    constexpr uint32_t kVerticesInRow = kCellsInRow + 1u;
    constexpr uint32_t kTileSize = 32u;
    constexpr float kTileScale = static_cast<float>(kTileSize) / kCellsInRow; // scale up the tile so that it occupies a 32x32 area with 31x31 cells
    
    Terrain::Terrain(TerrainSceneDelegate & sceneDelegate, engine::scene::Scene & scene, uint8_t w, uint8_t h) :
    _scene(scene),
    _width(w),
    _height(h),
    _tiles(w * h) {
        const auto terrainW = _width * kCellsInRow;
        const auto terrainH = _height * kCellsInRow;
        _terrainNode = scene.createNode();
        _terrainNode->setPivot({terrainW * 0.5f, terrainH * 0.5f, 0.0f}); // center of tile        
        _terrainNode->setBounds(
            engine::scene::Bounds::createCube(
                engine::vec3f{terrainW, terrainH, -static_cast<float>(kTileSize) * 0.5f}, 
                engine::vec3f{0.0f, 0.0f, static_cast<float>(kTileSize) * 0.5f}
            )
        );

        for (uint8_t i = 0u; i < _height; ++i) {
            for (uint8_t j = 0u; j < _width; ++j) {
                auto & tile = _tiles[i * _width + j];
                tile.terrainRowWidth = _width;
                tile.imageIndex = 0u;
                tile.x = j;
                tile.y = i;
                
                auto * tileNode = scene.createNode();
                tileNode->setPosition({j * kTileSize, i * kTileSize, 0.0f});

                tileNode->setBounds(
                    engine::scene::Bounds::createCube(
                        engine::vec3f{kTileSize, kTileSize, 0.0f}, 
                        engine::vec3f{0.0f, 0.0f, 0.0f}
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
    void TerrainSceneDelegate::destroy() noexcept {
        _tileCommonVertices = nullptr;
        _tileCommonIndices = nullptr;
        _tileIDs = nullptr;
        _updateNormalTileIDs = nullptr;
        _stagingBuffer = std::nullopt;

        for (auto & tm : _terrainMaps) {
            tm.clear();
        }
    }

    void TerrainSceneDelegate::buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device,
        uint16_t mapTilesWidth, uint16_t mapTilesHeight) {
        using namespace engine;
        using namespace engine::rhi;

        const auto mapImageWidth = mapTilesWidth * kTileSize;
        const auto mapImageHeight = mapTilesHeight * kTileSize;

        const BufferInfo vBufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const BufferInfo iBufferInfo {
            BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const BufferInfo idsBufferInfo {
            BufferUsageFlags::eUniform_Texel_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const ImageInfo normalsImageInfo{
            ImageUsageFlags::eStorage | ImageUsageFlags::eSampled,
            Format::eR8G8B8A8_Snorm, {mapImageWidth, mapImageHeight, 1u}
        };

        const ImageInfo heightsImageInfo{
            ImageUsageFlags::eTransfer_Dst | ImageUsageFlags::eStorage | ImageUsageFlags::eSampled,
            Format::eR16_Sfloat, {mapImageWidth, mapImageHeight, 1u}
        };

        _stagingBuffer.emplace(api, device, mapImageWidth * mapImageHeight * kFloat16Size);

        const size_t vBufferSize = kVerticesInRow * kVerticesInRow * sizeof(Vertex);
        const size_t iBufferSize = kCellsInRow * kCellsInRow * 6u * sizeof(uint16_t); // all cells count * 6u
        _tileCommonVertices = api.createBuffer(device, vBufferInfo, vBufferSize);
        _tileCommonIndices = api.createBuffer(device, iBufferInfo, iBufferSize);

        const size_t idsBufferSize = kMaxTileIds * sizeof(uint16_t);
        _tileIDs = api.createBuffer(device, idsBufferInfo, idsBufferSize);
        _updateNormalTileIDs = api.createBuffer(device, idsBufferInfo, idsBufferSize);

        auto & terrainMaps0 = _terrainMaps[0u];

        terrainMaps0.normals = api.createImage(device, normalsImageInfo);
        terrainMaps0.heights = api.createImage(device, heightsImageInfo);

        const auto flagsCount = static_cast<uint16_t>(mapImageWidth / kCellsInRow * mapImageHeight / kCellsInRow);
        terrainMaps0.initFlags(flagsCount, TerrainMaps::DirtyFlags::eNormals);
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
                        .position = {static_cast<float>(j) * kTileScale, static_cast<float>(i) * kTileScale}
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

    void TerrainSceneDelegate::uploadHeightData(engine::rhi::runtime::CommandList & commandList, engine::rhi::Format format,
        uint8_t mapIdx, engine::stl::span<const std::byte> bytes, engine::vec2u xy, engine::vec2u size) {
        using namespace engine;
        if (!_stagingBuffer.has_value()) {
            return;
        }

        auto & maps = _terrainMaps[mapIdx];
        const auto textureWidth = maps.normals->getInfo().extent.width;
        const auto tilesW = textureWidth / kTileSize;

        const auto startTile = xy / vec2u{kTileSize};
        const auto finishTile = (xy + size) / vec2u{kTileSize};

        for (auto y = startTile.y; y < finishTile.y; ++y) {
            for (auto x = startTile.x; x < finishTile.x; ++x) {
                const auto tileId = y * tilesW + x;
                maps.heightBounds[tileId].min = std::numeric_limits<int16_t>::max();
                maps.heightBounds[tileId].max = std::numeric_limits<int16_t>::lowest();
            }
        }

        const uint32_t pixelsCount = size.x * size.y;
        auto & staging = *_stagingBuffer;
        std::byte * memory = staging.map();
        const auto offset = staging.requestBytes(pixelsCount * kFloat16Size);
        auto * dst = reinterpret_cast<uint16_t*>(memory + offset);
        constexpr float hScale = 0.25f;

        for (uint32_t i = 0u; i < size.y; ++i) {
            for (uint32_t j = 0u; j < size.x; ++j) {
                const auto b = j + i * textureWidth;
                uint32_t srcByteId = b * 4u; // format depended
                const float value = static_cast<float>(bytes[srcByteId + 0u]) * hScale;
                // const float value = (static_cast<float>(bytes[srcByteId + 0u]) +
                //                 static_cast<float>(bytes[srcByteId + 1u]) +
                //                 static_cast<float>(bytes[srcByteId + 2u]) +
                //                 static_cast<float>(bytes[srcByteId + 3u])) * hScale;
                dst[b] = graphics::float_to_uint16(value);

                { // flags and heights
                    const uint32_t x = j + xy.x;
                    const uint32_t y = i + xy.y;

                    const auto tileX = x / kTileSize;
                    const auto tileY = y / kTileSize;
                    const auto tileId = tileY * tilesW + tileX;

                    const auto valuei = static_cast<int16_t>(value + 0.5f);
                    maps.heightBounds[tileId].min = std::min(maps.heightBounds[tileId].min, valuei);
                    maps.heightBounds[tileId].max = std::max(maps.heightBounds[tileId].max, valuei);
                    maps.dirty[tileId].enable(TerrainMaps::DirtyFlags::eNormals);
                }
            }
        }

        commandList.copyBuffer2Image(
            staging.getBuffer(), maps.heights, 
            {
                .bufferOffset = offset,
                .bufferRowLength = textureWidth,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = rhi::ImageAspectFlags::eColor},
                .imageOffset = {static_cast<int32_t>(xy.x), static_cast<int32_t>(xy.y), 0},
                .imageExtent = {size.x, size.y, 1u}
            }
        );
    }

    bool TerrainSceneDelegate::compare(
        const engine::scene::CustomSceneObject & object0, 
        const engine::scene::CustomSceneObject & object1
    ) const noexcept {
        using namespace engine;
        const auto & tile0 = static_cast<const TerrainTile2 &>(object0.getImpl());
        const auto & tile1 = static_cast<const TerrainTile2 &>(object1.getImpl());
        
        if (tile0.imageIndex != tile1.imageIndex) {
            return tile0.imageIndex < tile1.imageIndex;
        }

        const auto * p0 = object0.node->parent;
        const auto * p1 = object1.node->parent;

        return std::less<const scene::Node*>{}(p0, p1);
    }

    void TerrainSceneDelegate::beginFrame(std::chrono::time_point<std::chrono::steady_clock> now) {
        _tmpTilesCount = 0u;
        _tmpDirtyNormalTilesCount = 0u;
        _tileIdsDirty = false;
        _hasDirtyNormals = false;
    }

    void TerrainSceneDelegate::endFrame(
        engine::rhi::runtime::CommandListPtr & commandList, 
        engine::rhi::runtime::CommandListPtr & transferCommandList, 
        uint8_t frame) {
        using namespace engine;

        if (_stagingBuffer.has_value()) {
            _stagingBuffer->flush();
        }

        const auto targetSize = _tmpTilesCount * 2u;
        if (_visibleTileIds.size() > targetSize) {
            _visibleTileIds.resize(targetSize);
        }
        if (_dirtyNormalTileIds.size() > targetSize) {
            _dirtyNormalTileIds.resize(targetSize);
        }

        if (_tileIdsDirty) {
            const auto bufferUpdateDataSize = align_value(_tmpTilesCount * sizeof(uint16_t), 4u);
            assert(bufferUpdateDataSize <= kMaxTileIds * sizeof(uint16_t));
            transferCommandList->updateBuffer(_tileIDs, 0u, bufferUpdateDataSize, _visibleTileIds.data());
        }
        if (_hasDirtyNormals) {
            const auto bufferUpdateDataSize = align_value(_tmpDirtyNormalTilesCount * sizeof(uint16_t), 4u);
            assert(bufferUpdateDataSize <= kMaxTileIds * sizeof(uint16_t));
            transferCommandList->updateBuffer(_updateNormalTileIDs, 0u, bufferUpdateDataSize, _dirtyNormalTileIds.data());
        }
    }

    void TerrainSceneDelegate::dispatchNormals(const engine::scene::Scene & scene, engine::rhi::runtime::CommandListPtr & commandList, 
        uint8_t imageId, uint16_t offset, uint16_t count, uint8_t tileInRaw) {
        using namespace engine;

        constexpr uint16_t kTileLocalGroupSize = kTileSize;
        const auto verticesCount = kVerticesInRow * count; 
        const auto groupX = div_ceil(verticesCount, kTileLocalGroupSize);
        const auto groupY = div_ceil(verticesCount, kTileLocalGroupSize);

        const auto & maps = _terrainMaps[imageId];

        const struct Uniform {
            uint32_t idsBufferOffset = 0u;
            uint32_t tilesInRaw = 0u;
            uint32_t groupWidth = 0u;
        } uniform = {
            .idsBufferOffset = offset,
            .tilesInRaw = tileInRaw,
            .groupWidth = static_cast<uint32_t>(groupX)
        };

        const auto * pipeline = _pipelines[std::to_underlying(PipelineId::eNormalsCompute)];
        assert(pipeline != nullptr);

        commandList->bindPipeline(*pipeline);
        commandList->pushConstants(rhi::shaders::Stages::eCompute, 0u, sizeof(Uniform), &uniform);
        commandList->bindTexelBuffer(0u, _updateNormalTileIDs, rhi::Format::eR16_Uint, 0u);
        commandList->bindImage(1u, maps.heights, scene.getLinearSampler(), 0u);
        commandList->bindStorageImage(2u, maps.normals, std::nullopt, 0u);
        commandList->dispatch(groupX, groupY, 1u);
    }

    void TerrainSceneDelegate::dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end) {

        const std::ptrdiff_t objectsCount = end - begin;
        const auto targetSize = _tmpTilesCount + objectsCount;
        if (_visibleTileIds.size() < targetSize) {
            _visibleTileIds.resize(targetSize);
            _tileIdsDirty = true;
        }

        if (_dirtyNormalTileIds.size() < targetSize) {
            _dirtyNormalTileIds.resize(targetSize);
        }

        auto currentIt = begin;
        const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*currentIt);
        const auto & tile = static_cast<const TerrainTile2 &>(object.getImpl());
        const auto tileId = tile.y * tile.terrainRowWidth + tile.x;
        auto imageId = tile.imageIndex;

        if (_visibleTileIds[_tmpTilesCount] != tileId) {
            _visibleTileIds[_tmpTilesCount] = tileId;
            _tileIdsDirty = true;
        }
        ++_tmpTilesCount;

        uint16_t dispatchNormalsCounter = 0u;
        bool hasDirtyNormals = false;
        auto * maps = &_terrainMaps[tile.imageIndex];
        uint16_t offset = _tmpDirtyNormalTilesCount;
        if (maps->dirty[tileId] & TerrainMaps::DirtyFlags::eNormals) {
            _dirtyNormalTileIds[_tmpDirtyNormalTilesCount] = tileId;
            maps->dirty[tileId].disable(TerrainMaps::DirtyFlags::eNormals);
            ++_tmpDirtyNormalTilesCount;
            dispatchNormalsCounter = 1u;
            hasDirtyNormals = true;

            const auto & [minH, maxH] = maps->heightBounds[tileId];
            object.node->setBounds(
                engine::scene::Bounds::createCube(
                    engine::vec3f{kTileSize, kTileSize, minH}, 
                    engine::vec3f{0.0f, 0.0f, maxH}
                )
            );
        }

        for (auto it = begin + 1; it != end; ++it) {    
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*it);
            const auto & tile = static_cast<const TerrainTile2 &>(object.getImpl());
            const auto tileId = tile.y * tile.terrainRowWidth + tile.x;
            if (_visibleTileIds[_tmpTilesCount] != tileId) {
                _visibleTileIds[_tmpTilesCount] = tileId;
                _tileIdsDirty = true;
            }

            maps = &_terrainMaps[tile.imageIndex];
            if (maps->dirty[tileId] & TerrainMaps::DirtyFlags::eNormals) {
                if (tile.imageIndex != imageId) {
                    dispatchNormals(scene, commandList, imageId, offset, dispatchNormalsCounter, tile.terrainRowWidth);
                    imageId = tile.imageIndex;
                    dispatchNormalsCounter = 0u;
                    offset = _tmpDirtyNormalTilesCount;
                } 

                _dirtyNormalTileIds[_tmpDirtyNormalTilesCount] = tileId;
                maps->dirty[tileId].disable(TerrainMaps::DirtyFlags::eNormals);
                ++_tmpDirtyNormalTilesCount;
                ++dispatchNormalsCounter;
                hasDirtyNormals = true;

                const auto & [minH, maxH] = maps->heightBounds[tileId];
                object.node->setBounds(
                    engine::scene::Bounds::createCube(
                        engine::vec3f{kTileSize, kTileSize, minH}, 
                        engine::vec3f{0.0f, 0.0f, maxH}
                    )
                );
            }

            ++_tmpTilesCount;
        }

        if (hasDirtyNormals) {
            dispatchNormals(scene, commandList, imageId, offset, dispatchNormalsCounter, tile.terrainRowWidth);
        }

        _hasDirtyNormals |= hasDirtyNormals;
    }

    void TerrainSceneDelegate::drawSame(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end,
        uint32_t instanceOffset, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams,
        engine::scene::RenderPass pass, engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        switch (pass) {
        case engine::scene::RenderPass::eCommon:
        {
            struct Uniform {
                engine::mat4f model;
                uint32_t instanceOffset;
                uint32_t tileSize;
                uint32_t width;
                uint32_t tileUvSize;
            };

            const auto & gs = scene.getGraphicsService();
            const auto * pipeline = _pipelines[std::to_underlying(PipelineId::eTilesDraw)];
            assert(pipeline != nullptr);

            const auto objectsCount = static_cast<uint32_t>(end - begin);
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*begin);
            const auto & tile = static_cast<const TerrainTile2 &>(object.getImpl());
            const auto & terrainNode = *object.node->parent;

            Uniform uniform {
                .model = terrainNode.getWorldTransform(),
                .instanceOffset = instanceOffset,
                .tileSize = kTileSize,
                .width = tile.terrainRowWidth,
                .tileUvSize = 16u
            };

            if (commandList->bindPipeline(*pipeline)) {
                auto & maps = _terrainMaps[tile.imageIndex];
                auto const sceneViewParamBufferOffset = scene.getCurrentViewParamsBufferOffset();
                commandList->bindBuffer(0u, scene.getViewParamsUniformBuffer(frame), 0u, sceneViewParamBufferOffset);
                commandList->bindTexelBuffer(1u, _tileIDs, engine::rhi::Format::eR16_Uint, 0u);
                commandList->bindImage(2u, maps.heights, std::nullopt, 0u);
                commandList->bindImage(3u, maps.normals, scene.getLinearSampler(), 0u);
                commandList->bindImage(4u, gs.getTextureManager().getWhiteImage(), scene.getLinearSampler(), 0u);
            }

            commandList->pushConstants(
                engine::rhi::shaders::Stages::eVertex, 
                0u, sizeof(Uniform), &uniform
            );

            commandList->bindVertexBuffer(0u, _tileCommonVertices);
            commandList->bindIndexBuffer(_tileCommonIndices, 0u, engine::rhi::IndexType::eU16);
            commandList->drawIndexed(_tileCommonIndices->sizeInBytes() / sizeof(uint16_t), objectsCount);
        }
            break;
        case engine::scene::RenderPass::eShadow:
            break;
        case engine::scene::RenderPass::eColorId:
            break;
        }
    }

    void TerrainSceneDelegate::draw(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, 
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame) {

        auto currentIt = begin;
        const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*currentIt);
        const auto & tile = static_cast<const TerrainTile2 &>(object.getImpl());
        auto imageId = tile.imageIndex;
        auto * parentNode = object.node->parent;
        uint32_t instanceOffset = 0u;

        for (auto it = begin + 1; it != end; ++it) {
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*it);
            const auto & tile = static_cast<const TerrainTile2 &>(object.getImpl());
            auto currentImageId = tile.imageIndex;
            auto * currentParentNode = object.node->parent;

            if (imageId != currentImageId || parentNode != currentParentNode) {
                drawSame(currentIt, it, instanceOffset, scene, viewParams, pass, commandList, frame);
                imageId = currentImageId;
                parentNode = currentParentNode;
                instanceOffset += static_cast<uint32_t>(it - currentIt);
                currentIt = it;
            }
        }

        drawSame(currentIt, end, instanceOffset, scene, viewParams, pass, commandList, frame);
    }

}