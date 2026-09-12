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
    void setGraphicsPipeline(engine::rhi::runtime::Pipeline * p) noexcept {
        _pipeline = p;
    }

    void setComputePipeline(engine::rhi::runtime::Pipeline * p) noexcept {
        _computePipeline = p;
    }
    
    engine::rhi::runtime::Pipeline * getGraphicsPipeline() const noexcept {
        return _pipeline;
    }

    engine::rhi::runtime::Pipeline * getComputePipeline() const noexcept {
        return _computePipeline;
    }

    const engine::rhi::runtime::BufferPtr & getVerticesBuffer() const noexcept {
        return _vertices;
    }

    const engine::rhi::runtime::BufferPtr & getIndicesBuffer() const noexcept {
        return _indices;
    }

    uint16_t getSize() const noexcept { return _size; }
    uint16_t getSizeInCells() const noexcept { return _sizeInCells; }

    void destroy() noexcept;

private:
    engine::rhi::runtime::Pipeline * _pipeline = nullptr;
    engine::rhi::runtime::Pipeline * _computePipeline = nullptr;
    engine::rhi::runtime::BufferPtr _vertices = nullptr;
    engine::rhi::runtime::BufferPtr _indices = nullptr;
    uint16_t _size;
    uint8_t _sizeInCells;
};

class TerrainTileRenderer : public engine::scene::ICustomDrawObjectRenderer {
public:
    TerrainTileRenderer() : engine::scene::ICustomDrawObjectRenderer(true) {}
    void draw(
        const engine::scene::CustomDrawObject & object, const engine::scene::Scene & scene, const engine::scene::ViewParams & viewParams, 
        engine::scene::RenderPass pass, const engine::mat4f & worldTransform, 
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
    ) const override;

    void dispatch(
        const engine::scene::CustomDrawObject & object, const engine::scene::Scene & scene, const engine::mat4f & worldTransform,
        engine::rhi::runtime::CommandListPtr & commandList, uint8_t frameId
    ) const override;
};

}