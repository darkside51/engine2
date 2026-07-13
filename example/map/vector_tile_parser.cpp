#include "vector_tile_parser.h"

#include "engine/src/math/math.h"
#include "engine/src/math/earcut.h"
#include "engine/src/stl/algorithm.h"
#include "engine/src/utils/compression.h"
#include "engine/src/stl/helper.h"

#include "vtzero/vector_tile.hpp"
#include "vtzero/property_value.hpp"

#include <string_view>

namespace map {

    // define a variant covering the valid Vector Tile property types
    using variant_type = std::variant<std::string, float, double, int64_t, uint64_t, bool>;
    auto stringify = [](vtzero::property_value pvalue)->std::string {
        variant_type v = vtzero::convert_property_value<variant_type>(pvalue);
        return variant::visit(
            v, 
            [](std::string & value) { return value; },
            [](auto && value) { return std::to_string(value); }
        );
    };

    class GeometryHandler {
        template <typename T>
        using vector = engine::stl::vector<T>;

    public:
        explicit GeometryHandler(engine::vec2f extent) : _extent{extent} {}

        void points_begin(uint32_t count) { /*printf("Points [%d]", count);*/ }
        void points_point(const vtzero::point p) noexcept { /*printf("(%d, %d), ", p.x, p.y);*/ }
        void points_end() noexcept { /*printf("\n");*/ }

        void linestring_begin(uint32_t count) { /*printf("Line [%d]", count);*/ }
        void linestring_point(const vtzero::point p) noexcept { /*printf("(%d, %d), ", p.x, p.y);*/ }
        void linestring_end() noexcept { /*printf("\n");*/ }

        void ring_begin(uint32_t count) { _rings.emplace_back().reserve(count); }
        void ring_point(const vtzero::point p) noexcept {
            _rings.back().emplace_back(engine::vec2f{p.x, p.y} / _extent);
        }
        void ring_end(vtzero::ring_type) noexcept { 
            
        }

        vector<vector<engine::vec2f>> getRings() noexcept { return std::move(_rings); }

    private:
        engine::vec2f _extent{};
        vector<vector<engine::vec2f>> _rings;
    };

    void LayerPolygons::addGeometry(std::vector<uint32_t> indices, engine::stl::vector<engine::vec2f> vertices, float minHeight, float height) {
        // dirty
        totalIndices.reserve(totalIndices.size() + indices.size());
        const auto currentVertexCount = totalVerices.size();
        for (auto idx : indices) {
            totalIndices.emplace_back(currentVertexCount + idx);
        }

        totalVerices.reserve(currentVertexCount + vertices.size());
        for (auto vtx : vertices) {
            totalVerices.emplace_back(engine::vec3f{vtx.x, vtx.y, 0.0f});
        }
        // dirty
    }

    TileParsedGeometry VectorTileParser::parseGeometry(engine::stl::vector<std::byte> && parsedData) {
        auto data = engine::compression::isDataCompressed(parsedData) ? 
                    engine::compression::decompress(parsedData) : 
                    std::move(parsedData);

        const vtzero::data_view view{reinterpret_cast<char*>(data.data()), data.size()};
        vtzero::vector_tile tile{view};

        TileParsedGeometry geometry;
        auto & totalPolygons = geometry.polygons;
        while (auto layer = tile.next_layer()) {
            printf("layer name: %s\n", layer.name());

            LayerPolygons & polygons = totalPolygons.emplace_back();
            while (auto feature = layer.next_feature()) { //
                //printf("feature id: %lu\n", feature.id());

                // read feature tags (metadata attributes)
                //printf("Properties:\n");
                float height = 0.0f;
                float minHeight = 0.0f;

                while (auto property = feature.next_property()) {
                    auto str = property.key();
                    if (str == "height") {
                        auto value = property.value();
                        height = static_cast<float>(value.double_value());
                    }
                    if (str == "height_min") {
                        auto value = property.value();
                        minHeight = static_cast<float>(value.double_value());
                    }
                    printf("%s = %s\n", property.key().to_string().c_str(), stringify(property.value()).c_str());
                }

                if (height == 0.0f) {
                    continue;
                }

                // decode Geometries (Points, Lines, or Polygons)
                GeometryHandler handler{engine::vec2f{static_cast<float>(layer.extent())}};
                vtzero::decode_geometry(feature.geometry(), handler);
                auto rings = std::move(handler.getRings());
                if (!rings.empty()) {
                    auto indices = engine::earcut<uint32_t>(rings);
                    engine::stl::vector<engine::vec2f> vertices;
                    for (auto & ring : rings) {
                        vertices.reserve(vertices.size() + ring.size());
                        std::move(ring.begin(), ring.end(), engine::stl::back_inserter(vertices));
                    }

                    polygons.addGeometry(std::move(indices), std::move(vertices), minHeight, height);
                }
            }
        }

        return geometry;
    }
    
}