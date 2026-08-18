#include "vector_tile_parser.h"

#include "colors.h"
#include "geography.h"
#include "roofs.h"

#include "engine/src/common/random.h"
#include "engine/src/math/math.h"
#include "engine/src/math/earcut.h"
#include "engine/src/stl/algorithm.h"
#include "engine/src/utils/compression.h"
#include "engine/src/stl/helper.h"
#include "engine/src/log/log.h"

#include "vtzero/vector_tile.hpp"
#include "vtzero/property_value.hpp"

#include <string_view>

// test
#include <string>
#include <unordered_set>

namespace map {

    constexpr float kWallsDistanceFactor = 256.0f;

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
            std::vector<engine::vec2i> vertices;
            std::vector<uint32_t> indices;
            std::vector<std::pair<uint32_t, uint32_t>> rings;
        };

        explicit PolygonGeometryHandler(float extent) : _extent{extent} {}

        void ring_begin(uint32_t count) {
            // Подготавливаем буфер под новые вершины кольца
            _currentRing.clear();
            _currentRing.reserve(count);
        }

        void ring_point(const vtzero::point p) noexcept {
            _currentRing.emplace_back(engine::vec2i{p.x, p.y});
        }

        void ring_end(vtzero::ring_type type) {
            if (type == vtzero::ring_type::outer) {
                // Если пошел очередной outer-контур, значит предыдущий полигон 
                // в рамках MultiPolygon полностью завершен. Сначала триангулируем его.
                triangulateCurrentPolygon();
            }

            if (_currentRing.empty()) return;

            // MVT принудительно дублирует последнюю вершину для замыкания (P_last == P_first).
            // Для earcut этот дубликат вреден (вырождает треугольники), но для СТЕН он необходим.
            // Оставляем вершину, но earcut скорректирует её сам, если мы правильно развернем кольцо.
            
            // Переворачиваем кольцо: MVT дает CW для outer и CCW для inner. 
            // Earcut требует строго обратного.
            // std::reverse(_currentRing.begin(), _currentRing.end());

            // Запоминаем индексы дыр для earcut
            if (type == vtzero::ring_type::inner) {
                _accumulatedHoleIndices.emplace_back(static_cast<uint32_t>(_accumulatedVertices.size()));
            }

            // Накапливаем данные для текущего полигона
            _earcutPolygonStructure.emplace_back(_currentRing);
            _accumulatedVertices.insert(_accumulatedVertices.end(), _currentRing.begin(), _currentRing.end());
        }

        void finalize() {
            // Обрабатываем самый последний полигон фичи
            triangulateCurrentPolygon();
        }

        Polygon&& getPolygon() noexcept { return std::move(_polygon); }

    private:
        float _extent{};
        Polygon _polygon;
        
        std::vector<engine::vec2i> _currentRing;
        
        // Буферы для сборки одного изолированного полигона (контур + все его дыры)
        std::vector<std::vector<engine::vec2i>> _earcutPolygonStructure;
        std::vector<engine::vec2i> _accumulatedVertices;
        std::vector<uint32_t> _accumulatedHoleIndices;

        void triangulateCurrentPolygon() {
            if (_earcutPolygonStructure.empty()) {
                return;
            }

            const auto vertexStart = static_cast<uint32_t>(_polygon.vertices.size());
            
            // Нарезаем метаданные колец (нужно для построения стен в addGeometry)
            uint32_t currentRingOffset = 0u;
            for (const auto & ring : _earcutPolygonStructure) {
                const auto ringSize = static_cast<uint32_t>(ring.size());
                _polygon.rings.emplace_back(std::pair{
                    vertexStart + currentRingOffset,
                    vertexStart + currentRingOffset + ringSize - 1u
                });
                currentRingOffset += ringSize;
            }

            // Вызов триангуляции (принимает вложенные векторы колец)
            const auto localIndices = engine::earcut<uint32_t>(_earcutPolygonStructure);

            if (!localIndices.empty()) {
                // Переносим вершины в глобальный контейнер фичи
                _polygon.vertices.insert(
                    _polygon.vertices.end(), 
                    std::make_move_iterator(_accumulatedVertices.begin()), 
                    std::make_move_iterator(_accumulatedVertices.end())
                );

                // Смещаем локальные индексы earcut на vertexStart
                _polygon.indices.reserve(_polygon.indices.size() + localIndices.size());
                for (auto localIdx : localIndices) {
                    _polygon.indices.emplace_back(vertexStart + localIdx);
                }
            }

            // Очищаем буферы текущего полигона для подготовки к следующему в MultiPolygon
            _earcutPolygonStructure.clear();
            _accumulatedVertices.clear();
            _accumulatedHoleIndices.clear();
        }
    };

    void LayerPolygons::addGeometry(
        TileID tileId,
        std::vector<uint32_t> indices, 
        std::vector<engine::vec2i> vertices, 
        std::vector<std::pair<uint32_t, uint32_t>> rings,
        int16_t extent,
        float minHeight, 
        float maxHeight,
        float distanceFactor,
        int16_t material,
        int16_t roofMaterial,
        uint32_t color,
        uint32_t roofColor,
        float roofHeight,
        RoofType roofType
    ) {
        if (vertices.empty() || indices.empty()) {
            return;
        }

        //
        const auto latlon = tileToGeo(tileId, 0.5f, 0.5f);
        const auto extentf = static_cast<float>(extent);

        const auto hMin = metersToTileUnits(minHeight, latlon.x, tileId.z, extentf);
        const auto hMax = metersToTileUnits(maxHeight, latlon.x, tileId.z, extentf);

        minTileHeight = std::min(hMin / extent, minTileHeight);
        maxTileHeight = std::max(hMax / extent, maxTileHeight);
        color |= material << 24u;
        roofColor |= roofMaterial << 24u;

        const auto vsz = vertices.size();
        const auto isz = indices.size();

        // bottom
        const auto currentVertexCount = totalVerices.size();
        totalIndices.reserve(totalIndices.size() + isz);
        totalVerices.reserve(totalVerices.size() + vsz);
        totalColors.reserve(totalColors.size() + vsz);

        for (auto vtx : vertices) {
            totalVerices.emplace_back(
                engine::stl::array<int16_t, 4u>{
                    static_cast<int16_t>(vtx.x), 
                    static_cast<int16_t>(extent - vtx.y), 
                    static_cast<int16_t>(hMin),
                    static_cast<int16_t>(0)
                }
            );
            totalColors.emplace_back(color);
        }

        if (hMax <= hMin) {
            for (auto idx : indices) {
                totalIndices.emplace_back(currentVertexCount + idx);
            }

            if (hMin == 0.0f) {
                return;
            }
        } else {
            for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
                totalIndices.emplace_back(currentVertexCount + *it);
            }

            // is 3d object if hMax > hMin, build other planes

            // top
            const auto currentVertexCount2 = totalVerices.size();
            totalIndices.reserve(totalIndices.size() + isz);
            totalVerices.reserve(totalVerices.size() + vsz);
            totalColors.reserve(totalColors.size() + vsz);

            // for (auto idx : indices) {
            //     totalIndices.emplace_back(currentVertexCount2 + idx);
            // }
            for (auto vtx : vertices) {
                totalVerices.emplace_back(
                    engine::stl::array<int16_t, 4u>{
                        static_cast<int16_t>(vtx.x), 
                        static_cast<int16_t>(extent - vtx.y), 
                        static_cast<int16_t>(hMax),
                        static_cast<int16_t>(0)
                    }
                );
                totalColors.emplace_back(color);
            }

            // faces
            for (const auto & ring : rings) {
                float l = 0.0f;
                for (size_t i = ring.first; i <= ring.second; ++i) {
                    const auto nextIdx = (i == ring.second) ? ring.first : (i + 1u);

                    const auto floorCurrent = static_cast<uint32_t>(currentVertexCount + i);
                    const auto floorNext = static_cast<uint32_t>(currentVertexCount + nextIdx);

                    const auto roofCurrent  = static_cast<uint32_t>(currentVertexCount2 + i);
                    const auto roofNext = static_cast<uint32_t>(currentVertexCount2 + nextIdx);

                    totalIndices.emplace_back(floorCurrent);
                    totalIndices.emplace_back(floorNext);
                    totalIndices.emplace_back(roofCurrent);
                    
                    totalIndices.emplace_back(roofCurrent);
                    totalIndices.emplace_back(floorNext);
                    totalIndices.emplace_back(roofNext);

                    if (i != ring.second) {
                        const auto v0 = engine::vec2f{totalVerices[floorCurrent][0u], totalVerices[floorCurrent][1u]};
                        const auto v1 = engine::vec2f{totalVerices[floorNext][0u], totalVerices[floorNext][1u]};
                        const float wallLength = std::round(glm::distance(v0, v1) / distanceFactor);
                        l += wallLength;
                        totalVerices[floorNext][3u] = (static_cast<int16_t>(l) << 4);
                        totalVerices[roofNext][3u] = totalVerices[floorNext][3u];
                    }
                }
            }
        }

        switch (roofType) {
        case RoofType::eMany:
        case RoofType::eMansard:
        case RoofType::eRound:
        case RoofType::eGabled:
        case RoofType::eHipped: {
            buildHippedRoof(
                indices, vertices, rings,
                totalIndices, totalVerices, totalColors,
                maxHeight, roofHeight, hMax,
                latlon, extentf, roofColor, tileId.z
            );
        }
        break;

        case RoofType::eDome: {
            buildDomeRoof(
                indices, vertices, rings,
                totalIndices, totalVerices, totalColors,
                maxHeight, roofHeight, hMax,
                latlon, extentf, roofColor, tileId.z
            );
        }
        break;

        case RoofType::eSkillion: {
            buildSkillionRoof(
                indices, vertices, rings,
                totalIndices, totalVerices, totalColors,
                maxHeight, roofHeight, hMax,
                latlon, extentf, roofColor, tileId.z
            );
        }
        break;

        case RoofType::ePlants: { // good?
            buildPlantsRoof(
                indices, vertices, rings,
                totalIndices, totalVerices, totalColors,
                maxHeight, roofHeight, hMax,
                latlon, extentf, roofColor, tileId.z
            );
        }
        break;

        case RoofType::ePyramidal: {
            buildPyramidalRoof(
                indices, vertices, rings,
                totalIndices, totalVerices, totalColors,
                maxHeight, roofHeight, hMax,
                latlon, extentf, roofColor, tileId.z
            );
        }
        break;

        default:
        {
            const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
            for (auto vtx : vertices) {
                totalVerices.emplace_back(
                    engine::stl::array<int16_t, 4u>{
                        static_cast<int16_t>(vtx.x), 
                        static_cast<int16_t>(extent - vtx.y), 
                        static_cast<int16_t>(hMax), 
                        static_cast<int16_t>(1)
                    }
                );
                totalColors.emplace_back(roofColor);
            }

            for (auto idx : indices) {
                totalIndices.emplace_back(upperPolygonStartIdx + idx);
            }
        }
            break;
        }
    }

    template <typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    template <Numeric T>
    T getNumericValue(const vtzero::property_value & value) noexcept {
        switch(value.type()) {
        case vtzero::property_value_type::bool_value:
            return static_cast<T>(value.bool_value());
        case vtzero::property_value_type::sint_value:
            return static_cast<T>(value.sint_value());
        case vtzero::property_value_type::uint_value:
            return static_cast<T>(value.uint_value());
        case vtzero::property_value_type::int_value:
            return static_cast<T>(value.int_value());
        case vtzero::property_value_type::double_value:
            return static_cast<T>(value.double_value());
        case vtzero::property_value_type::float_value:
            return static_cast<T>(value.float_value());
        case vtzero::property_value_type::string_value:
            return T{0};
        }
        return T{0};
    }

    struct MaterialEntry {
        std::string_view name;
        uint32_t id;
    };

    static constexpr MaterialEntry kBuildingMaterials[] = {
        {"plaster",             0},
        {"glass",               1},
        {"wood",                2},
        {"metal",               3},
        {"metal_plates",        3},
        {"granite",             4},
        {"marble",              5},
        {"vinyl",               5},
        {"limestone",           6},
        {"concrate",            7},
        {"concrete",            7},
        {"porcelain_stoneware", 8},
        {"cement_block",        9},
        {"cement_block;glass",  9},
        {"brick",               10},
        {"roof_tar_paper",      11},
        {"roof_metal",          12},
        {"roof_granite",        13},
        {"roof_roof_tiles",     14},
        {"roof_steel",          15},
        {"roof_default",        16},
        {"roof_glass",          17},
    };

    static constexpr MaterialEntry kPolygonMaterials[] = {
        {"asphalt",             0},
        {"grass",               1},
        {"wood",                2},
        {"playground",          3},
        {"residential",         4},
        {"shrub",               2},
        {"parking",             5},
        {"industrial",          6},
    };

    RoofType parseRoofType(std::string_view roofType) noexcept {
        if (roofType == "flat") {
            return RoofType::eFlat;
        }

        if (roofType == "dome") {
            return RoofType::eDome;
        }

        if (roofType == "gabled") {
            return RoofType::eGabled;
        }

        if (roofType == "many") {
            return RoofType::eMany;
        }

        if (roofType == "hipped") {
            return RoofType::eHipped;
        }

        if (roofType == "round") {
            return RoofType::eRound;
        }

        if (roofType == "skillion") {
            return RoofType::eSkillion;
        }

        if (roofType == "plants") {
            return RoofType::ePlants;
        }

        if (roofType == "pyramidal") {
            return RoofType::ePyramidal;
        }

        if (roofType == "mansard") {
            return RoofType::eMansard;
        }

        LOG_W("no found roof type for : %s", roofType.data());
        return RoofType::eFlat;
    }

    uint32_t parseColor(std::string_view value) noexcept {
        if (value.starts_with('#')) {
            return hexStringToUint32(value).color;
        } else {
            uint32_t color = 0x00'ff'ff'ffu;
            const auto delimeter = std::min(value.find(';'), value.find('\''));
            bool colorFoundResult = false;
            if (delimeter != std::string::npos) { // get first color
                const auto [c, result] = colorNameToUint32t({value.begin(), value.begin() + delimeter});
                //const auto [c, result] = colorNameToUint32t({str.begin() + delimeter + 1u, str.end()});
                color = c;
                colorFoundResult = result;
            } else {
                const auto [c, result] = colorNameToUint32t(value);
                color = c;
                colorFoundResult = result;
            }

            if (!colorFoundResult) {
                LOG_W("no found color uint32_t for : %s", value.data());
            }
            return color;
        }
    }

    uint16_t parseBuildingMaterial(std::string_view value, uint16_t defaultResult) noexcept {
        const auto it = std::find_if(
            std::begin(kBuildingMaterials),
            std::end(kBuildingMaterials),
            [&value] (const MaterialEntry & material) noexcept {
                return value == material.name;
            }
        );

        if (it != std::end(kBuildingMaterials)) {
            return it->id;
        } else {
            LOG_W("no found material uint32_t for : %s", value.data());
            return defaultResult;
        }
    }

    void VectorTileParser::parsePolygonsGeometry(TileID tileId, LayerPolygons & polygons, vtzero::feature & feature, int16_t extent) {
        PolygonGeometryHandler handler{static_cast<float>(extent)};
        vtzero::decode_polygon_geometry(feature.geometry(), handler);
        handler.finalize();

        float maxHeight = 0.0f;
        float minHeight = 0.0f;

        int16_t materialIdx = 0;
        int16_t materialIdx2 = 16;
        uint32_t color = 0x00'ff'ff'ffu;
        uint32_t roofColor = 0x00'ff'ff'ffu;
        RoofType roofType = RoofType::eFlat;
        float roofHeight = 0.0f;

        while (auto property = feature.next_property()) {
            auto str = property.key();
            if (str == "class") {
                const auto value = property.value().string_value().to_string();
                
                const auto it = std::find_if(
                    std::begin(kPolygonMaterials),
                    std::end(kPolygonMaterials),
                    [&value] (const MaterialEntry & material) noexcept {
                        return value == material.name;
                    }
                );

                if (it != std::end(kPolygonMaterials)) {
                    materialIdx = it->id;
                } else {
                    static std::unordered_set<std::string> classes;
                    if (!classes.contains(value)) {
                        classes.insert(value);
                        LOG_W("no found material uint32_t for : %s", value.c_str());
                    }
                }
            }

            if (str == "height" || str == "render_height") {
                maxHeight = getNumericValue<float>(property.value());
            }

            if (str == "height_min" || str == "render_min_height") {
                minHeight = getNumericValue<float>(property.value());
            }

            if (str == "building_colour") {
                const auto value = property.value().string_value().to_string();
                color = parseColor(value);
            }

            if (str == "building_material") {
                const auto value = property.value().string_value().to_string();
                materialIdx = parseBuildingMaterial(value, materialIdx);
            }

            if (str == "roof_colour") {
                const auto value = property.value().string_value().to_string();
                roofColor = parseColor(value);
            }

            if (str == "roof_shape") {
                const auto value = property.value().string_value().to_string();
                roofType = parseRoofType(value);
            }

            if (str == "roof_height") {
                roofHeight = getNumericValue<float>(property.value());
            }

            if (str == "roof_material") {
                const auto value = "roof_" + property.value().string_value().to_string();
                materialIdx2 = parseBuildingMaterial(value, materialIdx2);
            }

            if (str == "windows") {
                const auto value = property.value().string_value().to_string();
                LOG_W("windows key found, value : %s", value.c_str());
            }
        }

        auto && polygon = handler.getPolygon();
        polygons.addGeometry(
            tileId,
            std::move(polygon.indices), 
            std::move(polygon.vertices), 
            std::move(polygon.rings),
            extent,
            minHeight, 
            maxHeight,
            static_cast<float>(extent) / kWallsDistanceFactor,
            materialIdx,
            materialIdx2,
            color,
            roofColor,
            roofHeight,
            roofType
        );
    }

    TileParsedGeometry VectorTileParser::parseGeometry(TileID tileId, engine::stl::vector<std::byte> && parsedData) {
        auto data = engine::compression::isDataCompressed(parsedData) ? 
                    engine::compression::decompress(parsedData) : 
                    std::move(parsedData);

        const vtzero::data_view view{reinterpret_cast<char*>(data.data()), data.size()};
        vtzero::vector_tile tile{view};

        TileParsedGeometry geometry;
        auto & totalPolygons = geometry.polygons;
        totalPolygons.resize(5u);

        struct LayerIndex {
            std::string_view name;
            uint16_t index = 0u;
        };

        static constexpr LayerIndex kLayers[] = {
            {"building",        0},
            {"landuse",         1},
            {"landcover",       2},
            {"water-",          3},
            {"transportation",  4}
        };

        while (auto layer = tile.next_layer()) {
            
            //printf("layer name: %s\n", layer.name());

            LayerPolygons * polygons = nullptr;
            uint32_t layerIdx = 0u;

            while (auto feature = layer.next_feature()) { //
                //printf("feature id: %lu\n", feature.id());
                const auto geometryType = feature.geometry_type();

                geometry.extent = std::max(static_cast<float>(layer.extent()), geometry.extent);
                
                switch (geometryType) {
                case vtzero::GeomType::POLYGON: {
                    if (polygons == nullptr) {
                        const auto it = std::find_if(
                                            std::begin(kLayers),
                                            std::end(kLayers),
                                            [name = layer.name()] (const LayerIndex & l) noexcept {
                                                return name.to_string() == l.name;
                                            }
                                        );

                        if (it != std::end(kLayers)) {
                            // printf("layer name: %s\n", layer.name());
                            layerIdx = it->index;
                            auto & p = totalPolygons[layerIdx];
                            p.layer = layerIdx;
                            polygons = &p.geometry;
                        } else {
                            break;
                        }
                    }

                    parsePolygonsGeometry(tileId, *polygons, feature, layer.extent());

                    if (std::abs(polygons->maxTileHeight - polygons->minTileHeight) > 1e-7f) {
                        totalPolygons[layerIdx].flags |= kFeatureFlag3D;
                    }

                    geometry.tileMaxHeight = std::max(
                        polygons->maxTileHeight, 
                        geometry.tileMaxHeight
                    );
                }
                    break;
                default: break;
                }
                
                if (feature.geometry_type() != vtzero::GeomType::POLYGON) {
                    continue;
                }
            }
        }

        LOG_I("geometry polygon layers count = %d", geometry.polygons.size());

        return geometry;
    }
    
}