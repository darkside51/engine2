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

    // class GeometryHandler {
    //     template <typename T>
    //     using vector = engine::stl::vector<T>;

    // public:
    //     explicit GeometryHandler(engine::vec2f extent) : _extent{extent} {}

    //     void points_begin(uint32_t count) { /*printf("Points [%d]", count);*/ }
    //     void points_point(const vtzero::point p) noexcept { /*printf("(%d, %d), ", p.x, p.y);*/ }
    //     void points_end() noexcept { /*printf("\n");*/ }

    //     void linestring_begin(uint32_t count) { /*printf("Line [%d]", count);*/ }
    //     void linestring_point(const vtzero::point p) noexcept { /*printf("(%d, %d), ", p.x, p.y);*/ }
    //     void linestring_end() noexcept { /*printf("\n");*/ }

    //     void ring_begin(uint32_t count) { _rings.emplace_back().reserve(count); }
    //     void ring_point(const vtzero::point p) noexcept {
    //         _rings.back().emplace_back(10.0f * engine::vec2f{p.x, p.y} / _extent);
    //     }
    //     void ring_end(vtzero::ring_type t) noexcept { 
    //     }

    //     vector<vector<engine::vec2f>> getRings() noexcept { return std::move(_rings); }

    // private:
    //     engine::vec2f _extent{};
    //     vector<vector<engine::vec2f>> _rings;
    // };


    class PolygonGeometryHandler {
    public:
        struct Polygon {
            std::vector<engine::vec2f> vertices;
            std::vector<uint32_t> indices;
        };

        explicit PolygonGeometryHandler(float extent) : _extent{extent} {}

        void ring_begin(uint32_t count) {
            _currentVertices.reserve(_currentVertices.size() + count);
        }

        void ring_point(const vtzero::point p) noexcept {
            _currentVertices.emplace_back(engine::vec2f{p.x, p.y} / _extent);
        }

        void ring_end(vtzero::ring_type type) {
            switch (type) {
            case vtzero::ring_type::outer:
                // Если это новое внешнее кольцо, значит предыдущий полигон
                // (со всеми его дырами, если они были) полностью собран. Триангулируем его.
                triangulateCurrentPolygon();
                break;
            case vtzero::ring_type::inner:
                // Если это дыра, запоминаем индекс её начала
                _currentHoleIndices.emplace_back(static_cast<uint32_t>(_currentVertices.size()));
                break;
            }
        }

        // Финализация: обрабатываем последний полигон после завершения итерации vtzero
        void finalize() {
            triangulateCurrentPolygon();
        }

        Polygon && getPolygon() noexcept { return std::move(_polygon); }

    private:
        float _extent{};
        Polygon _polygon;
        
        // Временные структуры для текущего полигона (включая его дыры)
        std::vector<engine::vec2f> _currentVertices;
        std::vector<uint32_t> _currentHoleIndices;

        void triangulateCurrentPolygon() {
            if (_currentVertices.empty()) {
                return;
            }

            std::vector<std::vector<engine::vec2f>> polygon;
            
            size_t nextHoleIdx = 0u;
            std::vector<engine::vec2f> currentRing;
            currentRing.reserve(1024u);
            
            const auto holesCount = _currentHoleIndices.size();

            for (size_t i = 0u; i < _currentVertices.size(); ++i) {
                // Если дошли до индекса дыры, закрываем текущее кольцо и начинаем новое
                if (nextHoleIdx < holesCount && i == _currentHoleIndices[nextHoleIdx]) {
                    polygon.emplace_back(std::move(currentRing));
                    currentRing.clear();
                    currentRing.reserve(1024u);
                    ++nextHoleIdx;
                }
                currentRing.emplace_back(_currentVertices[i]);
            }
            if (!currentRing.empty()) {
                polygon.emplace_back(std::move(currentRing));
            }

            const auto localIndices = engine::earcut<uint32_t>(polygon);

            // Переносим локальные вершины в глобальный меш и корректируем индексы
            const auto vertexStart = _polygon.vertices.size();
            _polygon.vertices.insert(_polygon.vertices.end(), _currentVertices.begin(), _currentVertices.end());

            for (auto localIdx : localIndices) {
                _polygon.indices.push_back(static_cast<uint32_t>(vertexStart + localIdx));
            }

            // Сброс буферов для следующего полигона в MultiPolygon
            _currentVertices.clear();
            _currentHoleIndices.clear();
        }
    };

    void LayerPolygons::addGeometry(std::vector<uint32_t> indices, std::vector<engine::vec2f> vertices, float minHeight, float height) {
        // dirty
        const auto currentVertexCount = totalVerices.size();
        const auto vsz = vertices.size();
        const auto isz = indices.size();

        totalIndices.reserve(totalIndices.size() + indices.size());
        for (auto idx : indices) {
            totalIndices.emplace_back(currentVertexCount + idx);
        }

        totalVerices.reserve(currentVertexCount + vertices.size());
        for (auto vtx : vertices) {
            totalVerices.emplace_back(engine::vec3f{4.0f * vtx.x, 4.0f * (1.0f - vtx.y) + 10.0f, 0.01f});
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

                if (feature.geometry_type() != vtzero::GeomType::POLYGON) {
                    continue;
                }
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
                // GeometryHandler handler{engine::vec2f{static_cast<float>(layer.extent())}};
                // vtzero::decode_geometry(feature.geometry(), handler);

                PolygonGeometryHandler handler{static_cast<float>(layer.extent())};
                vtzero::decode_polygon_geometry(feature.geometry(), handler);
                handler.finalize();

                auto && polygon = handler.getPolygon();

                polygons.addGeometry(std::move(polygon.indices), std::move(polygon.vertices), minHeight, height);
            }
        }

        return geometry;
    }
    
}