#pragma once

#include "math/math.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>
#include <vector>

namespace map {

    struct LayerPolygons {
        void addGeometry(
            std::vector<uint32_t> indices, 
            std::vector<engine::vec2i> vertices, 
            std::vector<std::pair<uint32_t, uint32_t>> rings, 
            float minHeight, 
            float height
        );
        std::vector<uint32_t> totalIndices;
        engine::stl::vector<engine::stl::array<int16_t, 4u>> totalVerices;
    };

    struct TileParsedGeometry {
        engine::stl::vector<LayerPolygons> polygons;
    };

    class VectorTileParser {
    public:
        template <typename GeometryHandler>
        void parseData(engine::stl::vector<std::byte> && data, GeometryHandler && handler) {
            handler(parseGeometry(std::move(data)));
        }

    private:
        TileParsedGeometry parseGeometry(engine::stl::vector<std::byte> && data);
    };

}