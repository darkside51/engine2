#pragma once

#include "tile_id.h"

#include "math/math.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace vtzero {
    class feature;
}

namespace map {

    static constexpr uint8_t kFeatureFlagCommon = 0u;
    static constexpr uint8_t kFeatureFlag3D = 1u;

    template <typename T>
    struct GeometryOnLayer {
        T geometry;
        uint32_t layer = 0u;
        uint8_t flags = kFeatureFlagCommon;
    };

    struct LayerPolygons {
        void addGeometry(
            TileID tileId,
            std::vector<uint32_t> indices, 
            std::vector<engine::vec2i> vertices, 
            std::vector<std::pair<uint32_t, uint32_t>> rings, 
            int16_t extent,
            float minHeight, 
            float maxHeight,
            float distanceFactor,
            int16_t material
        );
        std::vector<uint32_t> totalIndices;
        engine::stl::vector<engine::stl::array<int16_t, 4u>> totalVerices;
        float minTileHeight = std::numeric_limits<float>::max();
        float maxTileHeight = std::numeric_limits<float>::lowest();
    };

    struct TileParsedGeometry {
        engine::stl::vector<GeometryOnLayer<LayerPolygons>> polygons;
        float tileMaxHeight = 0.0f;
        float extent = 0.0f;
    };

    class VectorTileParser {
    public:
        template <typename GeometryHandler>
        void parseData(TileID tileId, engine::stl::vector<std::byte> && data, GeometryHandler && handler) {
            handler(parseGeometry(tileId, std::move(data)));
        }

    private:
        void parsePolygonsGeometry(TileID tileId, LayerPolygons & polygons, vtzero::feature & feature, int16_t extent);
        TileParsedGeometry parseGeometry(TileID tileId, engine::stl::vector<std::byte> && data);
    };

}