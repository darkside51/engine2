#include "map_tile.h"

#include "engine/src/common/align.h"
#include "engine/src/graphics/common/graphics_service.h"
#include "engine/src/graphics/render/rhi/common/command_list.h"
#include "engine/src/graphics/scene/scene.h"
#include "engine/src/graphics/texture/texture_manager.h"
#include "engine/src/graphics/utils/vertex_processing.h"
#include "engine/src/math/geometry.h"
#include "engine/src/stl/array.h"
#include "engine/src/stl/vector.h"

#include <cstddef>
#include <limits>

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
        engine::rhi::runtime::CommandList & commandList, uint8_t frameId
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
        commandList.bindPipeline(*pipeline);
        commandList.pushConstants(engine::rhi::shaders::Stages::eCompute, 0u, sizeof(float), &time);
        commandList.bindStorageImage(0u, geometry.getHeightsImage(), std::nullopt, 0u);
        commandList.dispatch(groupX, groupY, 1u);

        pipeline = geometry.getComputeNormalsPipeline();
        assert(pipeline != nullptr);
        commandList.bindPipeline(*pipeline);
        commandList.bindStorageImage(0u, geometry.getNormalsImage(), std::nullopt, 0u);
        commandList.bindImage(1u, geometry.getHeightsImage(), scene.getLinearSampler(), 0u);
        commandList.dispatch(groupX, groupY, 1u);
    }

    void TerrainTileRenderer::draw(
            const engine::scene::CustomSceneObject & object, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, 
            engine::scene::RenderPass pass, const engine::mat4f & worldTransform, 
            engine::rhi::runtime::CommandList & commandList, uint8_t frameId
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

                if (commandList.bindPipeline(*pipeline)) {
                    commandList.pushConstants(
                        engine::rhi::shaders::Stages::eVertex | engine::rhi::shaders::Stages::eFragment, 
                        0u, 72u, &uniform
                    );
                    commandList.bindBuffer(0u, scene.getViewParamsUniformBuffer(frameId), 0u, sceneViewParamBufferOffset);
                }

                const auto & ib = geometry.getIndicesBuffer();

                commandList.bindStorageImage(1u, geometry.getHeightsImage(), std::nullopt, 0u);
                commandList.bindImage(2u, geometry.getNormalsImage(), scene.getLinearSampler(), 0u);
                commandList.bindImage(3u, gs.getTextureManager().getDefaultImage(), scene.getNearestSampler(), 0u);

                commandList.bindVertexBuffer(0u, geometry.getVerticesBuffer());
                commandList.bindIndexBuffer(ib, 0u, engine::rhi::IndexType::eU16);
                commandList.drawIndexed(ib->sizeInBytes() / sizeof(uint16_t));
            }
            break;
            case engine::scene::RenderPass::eShadow:
            break;
            default: 
            break;
        }
    }

    void TerrainTileRenderer::dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandList & commandList, uint8_t frame, 
        engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end) {
        for (auto it = begin; it != end; ++it) { // dirty!!!
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*it);
            dispatch(object, scene, object.node->getWorldTransform(), commandList, frame);
        }
    }

    void TerrainTileRenderer::draw(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, 
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandList & commandList, uint8_t frame) {
        for (auto it = begin; it != end; ++it) { // dirty!!!
            const auto & object = *static_cast<const engine::scene::CustomSceneObject*>(*it);
            draw(object, scene, viewParams, pass, object.node->getWorldTransform(), commandList, frame);
        }
    }


    /////////////////////////
    constexpr uint16_t kMaxTileIds = 4096u;
    constexpr size_t kFloat16Size = 2u;
    constexpr uint32_t kVerticesInRow = 17u;
    constexpr uint32_t kCellsInRow = kVerticesInRow - 1u;
    constexpr uint32_t kTileSize = 32u;
    constexpr float kTileScale = static_cast<float>(kTileSize) / kCellsInRow; // scale up the tile

    Terrain::Terrain(TerrainSceneDelegate & sceneDelegate, engine::scene::Scene & scene, uint8_t w, uint8_t h) :
    _scene(scene),
    _width(w),
    _height(h),
    _tiles(w * h) {
        const auto terrainW = _width * kTileSize;
        const auto terrainH = _height * kTileSize;
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

        terrainMaps0.normalMap = api.createImage(device, normalsImageInfo);
        terrainMaps0.heightMap = api.createImage(device, heightsImageInfo);
        terrainMaps0.heights.resize(mapImageWidth * mapImageHeight);

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

        _dirtyGeometry = false;
    }

    void TerrainSceneDelegate::uploadHeightData(engine::rhi::runtime::CommandList & commandList, engine::rhi::Format format,
        uint8_t mapIdx, engine::stl::span<const std::byte> bytes, engine::vec2u xy, engine::vec2u size) {
        using namespace engine;
        if (!_stagingBuffer.has_value()) {
            return;
        }

        auto & maps = _terrainMaps[mapIdx];
        const auto textureWidth = maps.normalMap->getInfo().extent.width;
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
        constexpr float hScale = 0.2f;

        for (uint32_t i = 0u; i < size.y; ++i) {
            for (uint32_t j = 0u; j < size.x; ++j) {
                const auto b = j + i * textureWidth;
                const uint32_t srcByte = b * 4u; // format depended
                const auto value = static_cast<float>(bytes[srcByte + 0u]) * hScale;
                // const float value = (static_cast<float>(bytes[srcByte + 0u]) +
                //                 static_cast<float>(bytes[srcByte + 1u]) +
                //                 static_cast<float>(bytes[srcByte + 2u]) +
                //                 static_cast<float>(bytes[srcByte + 3u])) * hScale;
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

                    const auto hOffset = x + y * textureWidth;
                    maps.heights[hOffset] = value;
                }
            }
        }

        commandList.copyBuffer2Image(
            staging.getBuffer(), maps.heightMap, 
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
        engine::rhi::runtime::CommandList & commandList, 
        engine::rhi::runtime::CommandList & transferCommandList, 
        uint8_t frame) {
        using namespace engine;

        if (_dirtyGeometry) { [[unlikely]]
            buildGeometry(transferCommandList);
        }

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
            transferCommandList.updateBuffer(_tileIDs, 0u, bufferUpdateDataSize, _visibleTileIds.data());
        }
        if (_hasDirtyNormals) {
            const auto bufferUpdateDataSize = align_value(_tmpDirtyNormalTilesCount * sizeof(uint16_t), 4u);
            assert(bufferUpdateDataSize <= kMaxTileIds * sizeof(uint16_t));
            transferCommandList.updateBuffer(_updateNormalTileIDs, 0u, bufferUpdateDataSize, _dirtyNormalTileIds.data());
        }
    }

    void TerrainSceneDelegate::dispatchNormals(const engine::scene::Scene & scene, engine::rhi::runtime::CommandList & commandList, 
        uint8_t imageId, uint16_t offset, uint16_t count, uint8_t tileInRaw) {
        using namespace engine;

        constexpr uint16_t kTileLocalGroupSize = kTileSize;
        const auto pointsCount = kTileLocalGroupSize * count; 
        const auto groupX = count;// == div_ceil(pointsCount, kTileLocalGroupSize);
        const auto groupY = count;// == div_ceil(pointsCount, kTileLocalGroupSize);

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

        commandList.bindPipeline(*pipeline);
        commandList.pushConstants(rhi::shaders::Stages::eCompute, 0u, sizeof(Uniform), &uniform);
        commandList.bindTexelBuffer(0u, _updateNormalTileIDs, rhi::Format::eR16_Uint, 0u);
        commandList.bindImage(1u, maps.heightMap, scene.getLinearSampler(), 0u);
        commandList.bindStorageImage(2u, maps.normalMap, std::nullopt, 0u);
        commandList.dispatch(groupX, groupY, 1u);
    }

    void TerrainSceneDelegate::dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandList & commandList, uint8_t frame, 
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
        engine::scene::RenderPass pass, engine::rhi::runtime::CommandList & commandList, uint8_t frame) {
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

            if (commandList.bindPipeline(*pipeline)) {
                auto & maps = _terrainMaps[tile.imageIndex];
                auto const sceneViewParamBufferOffset = scene.getCurrentViewParamsBufferOffset();
                commandList.bindBuffer(0u, scene.getViewParamsUniformBuffer(frame), 0u, sceneViewParamBufferOffset);
                commandList.bindTexelBuffer(1u, _tileIDs, engine::rhi::Format::eR16_Uint, 0u);
                commandList.bindImage(2u, maps.heightMap, std::nullopt, 0u);
                commandList.bindImage(3u, maps.normalMap, scene.getLinearSampler(), 0u);
                commandList.bindImage(4u, gs.getTextureManager().getWhiteImage(), scene.getLinearSampler(), 0u);
            }

            commandList.pushConstants(
                engine::rhi::shaders::Stages::eVertex, 
                0u, sizeof(Uniform), &uniform
            );

            commandList.bindVertexBuffer(0u, _tileCommonVertices);
            commandList.bindIndexBuffer(_tileCommonIndices, 0u, engine::rhi::IndexType::eU16);
            commandList.drawIndexed(_tileCommonIndices->sizeInBytes() / sizeof(uint16_t), objectsCount);
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
        engine::rhi::runtime::CommandList & commandList, uint8_t frame) {

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

    float TerrainSceneDelegate::getHeight(engine::vec2f p, uint8_t imageIndex) const noexcept {
        using namespace engine;

        auto & maps = _terrainMaps[imageIndex];
        const auto & extent = maps.normalMap->getInfo().extent;
        const auto textureWidth = extent.width;
        const auto textureHeight = extent.height;

        if (p.x < 0.0f || p.x > textureWidth || p.y < 0.0f || p.y > textureHeight) {
            return 0.0f;
        }

        constexpr auto kStep = kTileSize / kCellsInRow;
        const auto xx = (static_cast<uint32_t>(p.x) / kStep) * kStep;
        const auto yy = (static_cast<uint32_t>(p.y) / kStep) * kStep;

        vec2f v0;
        vec2f v1;
        vec2f v2;

        const auto p1 = p * (1.0f / kStep);
        const auto fx = fract(p1.x);
        const auto fy = fract(p1.y);

        const auto idx = (yy / kStep) * kVerticesInRow + (xx / kStep);
        // { // v1 - simple
        // if (idx & 1) {
        //     v0 = {xx, yy};
        //     v2 = {xx + kStep, yy + kStep};
        //     v1 = (fx > fy) ? vec2f{xx + kStep, yy} : vec2f{xx, yy + kStep};
        // } else {
        //     v0 = {xx, yy + kStep};
        //     v2 = {xx + kStep, yy};
        //     v1 = ((1.0f - fx) > fy) ? vec2f{xx, yy} : vec2f{xx + kStep, yy + kStep};
        // }
        // }
        { // v2 - more optimal?
            const auto cond_idx = ((yy / kStep) * kVerticesInRow + (xx / kStep)) & 1;
            const auto cond_f1  = (fx > fy);
            const auto cond_f2  = ((1.0f - fx) > fy);

            v0 = { xx, yy + kStep * (1 - cond_idx) };
            v2 = { xx + kStep, yy + kStep * cond_idx };

            const auto choose_x = cond_idx ? cond_f1 : (1 - cond_f2);
            const auto choose_y = cond_idx ? (1 - cond_f1) : (1 - cond_f2);

            v1 = { xx + kStep * choose_x, yy + kStep * choose_y };
        }

        const auto idxH0 = static_cast<uint32_t>(v0.x) + static_cast<uint32_t>(v0.y) * textureWidth;
        const auto idxH1 = static_cast<uint32_t>(v1.x) + static_cast<uint32_t>(v1.y) * textureWidth;
        const auto idxH2 = static_cast<uint32_t>(v2.x) + static_cast<uint32_t>(v2.y) * textureWidth;

        const auto denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
        assert(abs(denom) > 1e-7f); // the triangle projects onto a line in the XOY plane

        // calculate the barycentric weights
        const auto lambda1 = ((v1.y - v2.y) * (p.x - v2.x) + (v2.x - v1.x) * (p.y - v2.y)) / denom;
        const auto lambda2 = ((v2.y - v0.y) * (p.x - v2.x) + (v0.x - v2.x) * (p.y - v2.y)) / denom;
        const auto lambda3 = 1.0f - lambda1 - lambda2;

        const float result = lambda1 * maps.heights[idxH0] + lambda2 * maps.heights[idxH1] + lambda3 * maps.heights[idxH2];
        return result;
    }

    // ray casting along the XY grid
    bool castRay(const engine::vec3f & orig, const engine::vec3f & dir, 
        const engine::vec2i size, engine::stl::span<const float> heights, 
        engine::vec3f & result) noexcept {
        constexpr auto kCellSize = kTileScale;

        const auto getHeight = [heights, size](int x, int y) noexcept -> float {
            if (x < 0 || x >= size.x || y < 0 || y >= size.y) return 0.0f;
            return heights[y * size.x + x];
        };

        const auto getVertex = [&](int x, int y) noexcept -> engine::vec3f {
            x *= kCellSize;
            y *= kCellSize;
            return {x, y, getHeight(x, y)};
        };

        // transform starting coordinates into grid space (indices)
        const float startX = orig.x / kCellSize;
        const float startY = orig.y / kCellSize;

        int cx = static_cast<int>(std::floor(startX));
        int cy = static_cast<int>(std::floor(startY));

        // step direction along X and Y axes
        const int stepX = (dir.x > 0) ? 1 : ((dir.x < 0) ? -1 : 0);
        const int stepY = (dir.y > 0) ? 1 : ((dir.y < 0) ? -1 : 0);

        // ray distance to the nearest cell boundaries
        float tMaxX = (dir.x != 0) ? ((cx + (stepX > 0 ? 1 : 0)) - startX) / (dir.x / kCellSize) : 1e9f;
        float tMaxY = (dir.y != 0) ? ((cy + (stepY > 0 ? 1 : 0)) - startY) / (dir.y / kCellSize) : 1e9f;

        // distance the ray needs to travel to cross one full cell
        const float tDeltaX = (dir.x != 0) ? std::abs(1.0f / (dir.x / kCellSize)) : 1e9f;
        const float tDeltaY = (dir.y != 0) ? std::abs(1.0f / (dir.y / kCellSize)) : 1e9f;

        float t = 0.0f;
        
        // loop runs while the ray is inside the terrain grid boundaries
        while (cx >= 0 && cx < size.x - 1 && cy >= 0 && cy < size.y - 1) {
            
            // get the 4 vertices of the current quad on the XY grid
            const auto v00 = getVertex(cx, cy);
            const auto v10 = getVertex(cx + 1, cy);
            const auto v01 = getVertex(cx, cy + 1);
            const auto v11 = getVertex(cx + 1, cy + 1);

            // current height of the ray when entering this cell
            const float rayZ = orig.z + dir.z * t;
            
            // find the maximum height (Z) among the 4 cell vertices
            const float maxZ = std::max({v00.z, v10.z, v01.z, v11.z});
            
            // if the ray points downwards (dir.z < 0) and is still above the highest peak of this cell,
            // we can safely skip the precise triangle intersection test.
            // (a safety margin of +10.0f is needed if the ray is almost horizontal or moving upwards)
            if (dir.z > 0 || rayZ <= maxZ + 10.0f) { 
                float tHit;
                engine::vec3f hitPoint;
                bool hit = false;

                // check the first triangle of the quad
                if (engine::rayTriangleIntersect(orig, dir, v00, v10, v11, tHit, hitPoint)) {
                    result = hitPoint; 
                    hit = true;
                }
                // check the second triangle of the quad
                if (engine::rayTriangleIntersect(orig, dir, v00, v11, v01, tHit, hitPoint)) {
                    if (!hit || tHit < t) { 
                        result = hitPoint; 
                        hit = true;
                    }
                }

                if (hit) {
                    return true; // success
                }
            }

            // Amanatides-Woo algorithm: select which grid boundary the ray intersects first
            if (tMaxX < tMaxY) {
                t = tMaxX;
                tMaxX += tDeltaX;
                cx += stepX;
            } else {
                t = tMaxY;
                tMaxY += tDeltaY;
                cy += stepY;
            }
        }
        return false;
    }

    engine::vec3f TerrainSceneDelegate::getIntersection(const engine::interval3f & interval, uint8_t imageIndex) const noexcept {
        using namespace engine;

        const auto & maps = _terrainMaps[imageIndex];
        const auto & extent = maps.normalMap->getInfo().extent;
        const auto textureWidth = extent.width;
        const auto textureHeight = extent.height;

        engine::vec3f result;
        castRay(interval.begin, engine::math::normalize(interval.end - interval.begin), {textureWidth, textureHeight}, maps.heights, result);

        // LOG_I("intersectiorn point is: %fx%fx%f", result.x, result.y, result.z);

        return result;//vec3f{std::numeric_limits<float>::infinity()};
    }

}