#pragma once

#include "engine/src/common/flags.h"
#include "engine/src/graphics/render/rhi/common/runtime.h"
#include "engine/src/graphics/scene/objects/custom_object.h"
#include "engine/src/graphics/scene/node.h"
#include "engine/src/math/math.h"
#include "engine/src/stl/array.h"
#include "engine/src/stl/vector.h"

#include <cstdint>

namespace g_map {
    
class TerrainTile : public engine::scene::AttachmentImpl {
public:
    TerrainTile(uint16_t size, uint8_t sizeInCells);
    void buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device);
    void buildGeometry(engine::rhi::runtime::CommandList & cmd);
    void setGraphicsPipeline(engine::rhi::runtime::Pipeline * p) noexcept {
        _pipeline = p;
    }

    void setComputeHeightPipeline(engine::rhi::runtime::Pipeline * p) noexcept {
        _computePipelineHeight = p;
    }

    void setComputeNormalsPipeline(engine::rhi::runtime::Pipeline * p) noexcept {
        _computePipelineNormals = p;
    }
    
    engine::rhi::runtime::Pipeline * getGraphicsPipeline() const noexcept {
        return _pipeline;
    }

    engine::rhi::runtime::Pipeline * getComputeHeightPipeline() const noexcept {
        return _computePipelineHeight;
    }

    engine::rhi::runtime::Pipeline * getComputeNormalsPipeline() const noexcept {
        return _computePipelineNormals;
    }

    const engine::rhi::runtime::BufferPtr & getVerticesBuffer() const noexcept {
        return _vertices;
    }

    const engine::rhi::runtime::BufferPtr & getIndicesBuffer() const noexcept {
        return _indices;
    }

    const engine::rhi::runtime::ImagePtr & getNormalsImage() const noexcept {
        return _normals;
    }

    const engine::rhi::runtime::ImagePtr & getHeightsImage() const noexcept {
        return _heights;
    }

    uint16_t getSize() const noexcept { return _size; }
    uint16_t getSizeInCells() const noexcept { return _sizeInCells; }

    void destroy() noexcept;

private:
    engine::rhi::runtime::Pipeline * _pipeline = nullptr;
    engine::rhi::runtime::Pipeline * _computePipelineHeight = nullptr;
    engine::rhi::runtime::Pipeline * _computePipelineNormals = nullptr;
    engine::rhi::runtime::BufferPtr _vertices = nullptr;
    engine::rhi::runtime::BufferPtr _indices = nullptr;
    engine::rhi::runtime::ImagePtr _heights = nullptr;
    engine::rhi::runtime::ImagePtr _normals = nullptr;
    
    uint16_t _size;
    uint8_t _sizeInCells;
};

class TerrainTileRenderer : public engine::scene::ICustomSceneDelegate {
public:
    using object_type = TerrainTile;
    static constexpr auto kFlags = engine::scene::CustomSceneDelegateFlags::eDraw | engine::scene::CustomSceneDelegateFlags::eDispatch;

    TerrainTileRenderer() : engine::scene::ICustomSceneDelegate(kFlags) {}

    void draw(
        const engine::scene::CustomSceneObject & object, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, 
        engine::scene::RenderPass pass, const engine::mat4f & worldTransform, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
    ) const;

    void dispatch(
        const engine::scene::CustomSceneObject & object, const engine::scene::Scene & scene, const engine::mat4f & worldTransform,
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
    ) const;

    void dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end) override;

    void draw(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, 
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame) override;
};

///////////////////////
// revision
class Terrain;

// enum class DirtyFlags : uint8_t {

// };
// inline constexpr DirtyFlags operator | (DirtyFlags a, DirtyFlags b) noexcept {
//     return static_cast<DirtyFlags>(std::to_underlying(a) | std::to_underlying(b));
// }
// inline constexpr DirtyFlags operator & (DirtyFlags a, DirtyFlags b) noexcept {
//     return static_cast<DirtyFlags>(std::to_underlying(a) & std::to_underlying(b));
// }



struct TerrainTile2 : public engine::scene::AttachmentImpl {
    uint8_t terrainRowWidth = 0u;
    uint8_t imageIndex = 0u;
    uint8_t x = 0u;
    uint8_t y = 0u;
};

class TerrainSceneDelegate : public engine::scene::ICustomSceneDelegate {
    static constexpr auto kFlags = engine::scene::CustomSceneDelegateFlags::eDraw | engine::scene::CustomSceneDelegateFlags::eDispatch;

public:
    enum class PipelineId : uint8_t {
        eTilesDraw = 0u,
        eHeightsCompute = 1u,
        eNormalsCompute = 2u, 
        eCount
    };

    using object_type = TerrainTile2;
    
    TerrainSceneDelegate() : engine::scene::ICustomSceneDelegate(kFlags) {}
    ~TerrainSceneDelegate() {
        destroy();
    }

    void destroy() noexcept;

    void buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device, 
        uint16_t mapImageWidth, uint16_t mapImageHeight);
    void buildGeometry(engine::rhi::runtime::CommandList & cmd);

    void setPipeline(engine::rhi::runtime::Pipeline * p, PipelineId pid) noexcept {
        _pipelines[static_cast<uint8_t>(pid)] = p;
    }

    const engine::rhi::runtime::Pipeline * getPipeline(PipelineId pid) const noexcept {
        return _pipelines[static_cast<uint8_t>(pid)];
    }

    bool compare(const engine::scene::CustomSceneObject & object0, 
        const engine::scene::CustomSceneObject & object1) const noexcept override;

    void beginFrame(std::chrono::time_point<std::chrono::steady_clock> now);

    void endFrame(
        engine::rhi::runtime::CommandListPtr & commandList, 
        engine::rhi::runtime::CommandListPtr & transferCommandList, 
        uint8_t frame) override;

    void dispatch(const engine::scene::Scene & scene, engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end) override;

    void draw(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, 
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame) override;

    void drawSame(engine::scene::AttachmentsIterator begin, engine::scene::AttachmentsIterator end, uint32_t instanceOffset,
        const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, engine::scene::RenderPass pass, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame);

private:
    engine::stl::array<engine::rhi::runtime::Pipeline *, static_cast<uint8_t>(PipelineId::eCount)>  _pipelines;
    engine::rhi::runtime::BufferPtr _tileCommonVertices = nullptr;
    engine::rhi::runtime::BufferPtr _tileCommonIndices = nullptr;
    engine::rhi::runtime::BufferPtr _tileIDs = nullptr;

    struct TerrainMaps {
        enum class DirtyFlags : uint8_t {
            eNone = 0u,
            eHeights = 1u,
            eNormals = 2u,
            eSplat = 3u
        };

        engine::rhi::runtime::ImagePtr heights = nullptr;
        engine::rhi::runtime::ImagePtr normals = nullptr;
        engine::rhi::runtime::ImagePtr splatMap = nullptr;

        engine::stl::vector<engine::Flags<DirtyFlags>> dirty;

        void initFlags(uint16_t size, DirtyFlags value) {
            dirty.reserve(size);
            for (uint16_t i = 0u; i < size; ++i) {
                dirty[i] = value;
            }
        } 

        void clear() noexcept {
            dirty.clear();
            heights = nullptr;
            normals = nullptr;
            splatMap = nullptr;
        }
    };

    engine::stl::array<TerrainMaps, 16u> _terrainMaps; // 4 x 4 maps maximum
    engine::stl::vector<uint16_t> _visibleTileIds;
    engine::stl::vector<uint16_t> _dirtyNormalTileIds;
    uint16_t _mapImageMaxSize = 0u;
    uint16_t _tmpTilesCount = 0u;
    uint16_t _tmpDirtyNormalsTilesCount = 0u;
    bool _tileIdsDirty = false;
    bool _hasDirtyNormals = false;
};

class Terrain {
public:
    Terrain(TerrainSceneDelegate & sceneDelegate, engine::scene::Scene & scene, uint8_t w, uint8_t h);
    ~Terrain();

    engine::scene::Node * getNode() const noexcept {
        return _terrainNode;
    }

    uint8_t width() const noexcept { return _width; }
    uint8_t height() const noexcept { return _height; }

private:
    engine::scene::Scene & _scene;
    engine::scene::Node * _terrainNode = nullptr; // terrain root node
    engine::stl::vector<TerrainTile2> _tiles;
    uint8_t _width;
    uint8_t _height;
};


}