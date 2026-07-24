#pragma once

#include "tile_id.h"

#include "math/math.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>
#include <vector>

namespace map {

    struct LayerPolygons {
        float addGeometry(
            TileID tileId,
            std::vector<uint32_t> indices, 
            std::vector<engine::vec2i> vertices, 
            std::vector<std::pair<uint32_t, uint32_t>> rings, 
            int16_t extent,
            float minHeight, 
            float height,
            int16_t material
        );
        std::vector<uint32_t> totalIndices;
        engine::stl::vector<engine::stl::array<int16_t, 4u>> totalVerices;
        uint32_t layer = 0u;
    };

    struct TileParsedGeometry {
        engine::stl::vector<LayerPolygons> polygons;
        float height = 0.0f;
    };

    class VectorTileParser {
    public:
        template <typename GeometryHandler>
        void parseData(TileID tileId, engine::stl::vector<std::byte> && data, GeometryHandler && handler) {
            handler(parseGeometry(tileId, std::move(data)));
        }

    private:
        TileParsedGeometry parseGeometry(TileID tileId, engine::stl::vector<std::byte> && data);
    };

}