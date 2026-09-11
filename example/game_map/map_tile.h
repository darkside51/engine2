#pragma once

#include "engine/src/math/math.h"
#include "engine/src/graphics/render/rhi/common/runtime.h"
#include "engine/src/graphics/scene/objects/custom_draw_object.h"
#include <cstdint>

namespace g_map {
    
class TerrainTile {
public:
    TerrainTile(uint16_t size, uint8_t sizeInCells);
    void buildBuffers(engine::rhi::runtime::Rhi & api, engine::rhi::runtime::Device & device);
    void buildGeometry(engine::rhi::runtime::CommandList & cmd);
    void setPipeline(engine::rhi::runtime::Pipeline * p) noexcept {
        _pipeline = p;
    }
    
    engine::rhi::runtime::Pipeline * getPipeline() const noexcept {
        return _pipeline;
    }

    const engine::rhi::runtime::BufferPtr & getVerticesBuffer() const noexcept {
        return _vertices;
    }

    const engine::rhi::runtime::BufferPtr & getIndicesBuffer() const noexcept {
        return _indices;
    }

    uint16_t getSize() const noexcept { return _size; }

private:
    engine::rhi::runtime::Pipeline * _pipeline = nullptr;
    engine::rhi::runtime::BufferPtr _vertices = nullptr;
    engine::rhi::runtime::BufferPtr _indices = nullptr;
    uint16_t _size;
    uint8_t _sizeInCells;
};

class TerrainTileRenderer : public engine::scene::ICustomDrawObjectRenderer {
public:
    void draw(
            const engine::scene::CustomDrawObject & object, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, 
            engine::scene::RenderPass pass, const engine::mat4f & worldTransform, 
            engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
        ) const override;
};

}