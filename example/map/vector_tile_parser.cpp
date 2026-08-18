#include "vector_tile_parser.h"

#include "colors.h"
#include "geography.h"

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
            const float kMinHippedRoofHeightInMeters = std::min(maxHeight / 6.0f, 4.0f);
            const auto hRidge = hMax + metersToTileUnits(
                (roofHeight == 0.0f ? kMinHippedRoofHeightInMeters : roofHeight),
                latlon.x, tileId.z, extentf);
            
            // Фиксированный отступ ската в единицах тайла
            const float insetDistance = metersToTileUnits(1.5f, latlon.x, tileId.z, extentf);
            constexpr float kBottomOffset = 4.0f;

            // ==============================================================================
            // ПОЛИГОН 1: НИЖНИЙ ПОЛИГОН КРЫШИ (Карниз на высоте hMax)
            const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            // roof bottom plane
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

            // roof bottom plane indices
            for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
                totalIndices.emplace_back(lowerPolygonStartIdx + *it);
            }

            // ==============================================================================
            // ПОЛИГОН 2: ВЕРХНИЙ ПОЛИГОН КРЫШИ (Конёк на высоте hRidge, суженный по биссектрисам)
            const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            // Изначально заполняем верхний полигон чистыми копиями для сохранения структуры индексов Шага 2
            for (auto vtx : vertices) {
                totalVerices.emplace_back(
                    engine::stl::array<int16_t, 4u>{
                        static_cast<int16_t>(vtx.x), 
                        static_cast<int16_t>(extent - vtx.y), 
                        static_cast<int16_t>(hRidge), 
                        static_cast<int16_t>(1)
                    }
                );
                totalColors.emplace_back(roofColor);
            }

            // Создаем структуру для хранения новой геометрии конька под повторный Earcut
            // Формат зависит от вашего типа Point, предположим std::vector<std::vector<PointType>>
            // где первый элемент — внешнее кольцо, последующие — дыры.
            using Point2D = engine::vec2f; // Подставьте ваш тип точки, если нужен int/float
            std::vector<std::vector<Point2D>> earcutPolygonData;
            earcutPolygonData.reserve(rings.size());

            // Модифицируем вершины верхнего полигона по вашему лаконичному алгоритму
            for (const auto & ring : rings) {
                if (ring.second < ring.first) continue;

                bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                              vertices[ring.first].y == vertices[ring.second].y);
                size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;

                // Создаем локальное кольцо для триангулятора
                std::vector<Point2D> localNewRing;
                localNewRing.reserve((ring.second - ring.first) + 1);

                for (size_t i = ring.first; i <= ring.second; ++i) {
                    size_t prevIdx, nextIdx;
                    if (isClosedWithDuplicate && i == ring.second) {
                        prevIdx = endIdx;
                        nextIdx = ring.first + 1u;
                    } else {
                        prevIdx = (i == ring.first) ? endIdx : (i - 1u);
                        nextIdx = (i == endIdx) ? ring.first : (i + 1u);
                    }

                    const auto & vCurr = vertices[i];
                    const auto & vPrev = vertices[prevIdx];
                    const auto & vNext = vertices[nextIdx];

                    const engine::vec2f d1{vCurr.x - vPrev.x, vCurr.y - vPrev.y};
                    const engine::vec2f d2{vNext.x - vCurr.x, vNext.y - vCurr.y};

                    const engine::vec2f n1 = glm::normalize(engine::vec2f{-d1.y, d1.x});
                    const engine::vec2f n2 = glm::normalize(engine::vec2f{-d2.y, d2.x});

                    const engine::vec2f shiftDir = glm::normalize(n1 + n2);

                    const float dotProd = glm::dot(shiftDir, n1);
                    float length = insetDistance;
                    
                    if (std::abs(dotProd) > 0.1f) {
                        length = insetDistance / std::abs(dotProd);
                    }

                    if (length > insetDistance * 1.8f) {
                        length = insetDistance * 1.8f;
                    }

                    const float ridgePtX = static_cast<float>(vCurr.x) + shiftDir.x * length;
                    const float ridgePtY = static_cast<float>(vCurr.y) + shiftDir.y * length;

                    const int16_t finalX = static_cast<int16_t>(std::round(ridgePtX));
                    const int16_t finalY = static_cast<int16_t>(std::round(ridgePtY));

                    // Пишем сдвинутые координаты в глобальный буфер Полигона 2
                    totalVerices[upperPolygonStartIdx + i] = engine::stl::array<int16_t, 4u>{
                        finalX, 
                        static_cast<int16_t>(extent - finalY), 
                        static_cast<int16_t>(hRidge), 
                        static_cast<int16_t>(1)
                    };

                    // test
                    const float ridgePtX2 = static_cast<float>(vCurr.x) - shiftDir.x * kBottomOffset;
                    const float ridgePtY2 = static_cast<float>(vCurr.y) - shiftDir.y * kBottomOffset;

                    const int16_t finalX2 = static_cast<int16_t>(std::round(ridgePtX2));
                    const int16_t finalY2 = static_cast<int16_t>(std::round(ridgePtY2));

                    totalVerices[lowerPolygonStartIdx + i] = engine::stl::array<int16_t, 4u>{
                        finalX2, 
                        static_cast<int16_t>(extent - finalY2), 
                        static_cast<int16_t>(hMax), 
                        static_cast<int16_t>(1)
                    };
                    // test

                    // Сохраняем точку для повторного Earcut (в исходной системе координат)
                    localNewRing.emplace_back(Point2D{ridgePtX, ridgePtY});
                }

                earcutPolygonData.emplace_back(std::move(localNewRing));
            }

            // ==============================================================================
            // ШАГ 2: СТРОИМ СТЕНЫ КРЫШИ (Боковые покатые наклонные грани)
            // Строятся по локальным rings, связывая нижний и верхний монолитные буферы
            for (const auto & ring : rings) {
                if (ring.second < ring.first) continue;

                bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                              vertices[ring.first].y == vertices[ring.second].y);
                size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;

                for (size_t i = ring.first; i <= endIdx; ++i) {
                    const auto nextIdx = (i == endIdx) ? ring.first : (i + 1u);

                    const auto eCurrent = lowerPolygonStartIdx + static_cast<uint32_t>(i);
                    const auto eNext    = lowerPolygonStartIdx + static_cast<uint32_t>(nextIdx);
                    
                    const auto rCurrent = upperPolygonStartIdx + static_cast<uint32_t>(i);
                    const auto rNext    = upperPolygonStartIdx + static_cast<uint32_t>(nextIdx);

                    totalIndices.emplace_back(eCurrent);
                    totalIndices.emplace_back(eNext);
                    totalIndices.emplace_back(rCurrent);

                    totalIndices.emplace_back(rCurrent);
                    totalIndices.emplace_back(eNext);
                    totalIndices.emplace_back(rNext);
                }
            }

            // ==============================================================================
            // ШАГ 3: ПОВТОРНАЯ ТРИАНГУЛЯЦИЯ СЖАТЫХ ВЕРШИН
            // Передаем новый суженный контур здания в Earcut. 
            // Он построит чистые треугольники без вылетов во двор и без учета старой топологии.
            // Примечание: Подставьте точный вызов вашего Earcut-декодера, если интерфейс отличается.
            std::vector<uint32_t> newRidgeIndices = mapbox::earcut<uint32_t>(earcutPolygonData);

            for (auto idx : newRidgeIndices) {
                if (idx < vertices.size()) {
                    totalIndices.emplace_back(upperPolygonStartIdx + idx);
                }
            }
        }
        break;

        case RoofType::eDome: {
            if (rings.empty()) break;

            // 1. ВЫЧИСЛЕНИЕ ВЫСОТ
            const float kMinDomeHeightInMeters = 4.0f;
            const auto hRidge = hMax + metersToTileUnits(
                (roofHeight == 0.0f ? kMinDomeHeightInMeters : roofHeight),
                latlon.x, tileId.z, extentf);
            
            const float domeHeight = static_cast<float>(hRidge - hMax);

            // Внешнее кольцо здания
            const auto& outerRing = rings[0]; 
            bool isClosedWithDuplicate = (vertices[outerRing.first].x == vertices[outerRing.second].x && 
                                        vertices[outerRing.first].y == vertices[outerRing.second].y);
            size_t endIdx = isClosedWithDuplicate ? (outerRing.second - 1u) : outerRing.second;
            size_t numBaseVertices = endIdx - outerRing.first + 1;

            if (numBaseVertices < 3) break;

            // 2. НАХОДИМ ГЕОМЕТРИЧЕСКИЙ ЦЕНТР (AABB) ДЛЯ ПРОПОРЦИОНАЛЬНОГО СЖАТИЯ
            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();

            for (size_t i = outerRing.first; i <= endIdx; ++i) {
                float x = static_cast<float>(vertices[i].x);
                float y = static_cast<float>(vertices[i].y);
                if (x < minX) minX = x; if (x > maxX) maxX = x;
                if (y < minY) minY = y; if (y > maxY) maxY = y;
            }

            engine::vec2f center{(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};

            // 3. НАСТРОЙКА ДЕТАЛИЗАЦИИ КУПОЛА
            // Количество горизонтальных колец (ярусов) купола. Чем больше, тем плавнее купол.
            constexpr size_t kSegmentsZ = 8; 

            // Запоминаем стартовый индекс в глобальном буфере для генерации индексов треугольников
            const uint32_t domeStartGlobalIdx = static_cast<uint32_t>(totalVerices.size());

            // 4. ГЕНЕРАЦИЯ ВЕРШИН КУПОЛА (ПОСЛОЙНО СНИЗУ ВВЕРХ)
            // Мы идем от i = 0 (основание на высоте hMax) до i = kSegmentsZ - 1 (предпоследнее кольцо перед пиком)
            for (size_t ringIdx = 0; ringIdx < kSegmentsZ; ++ringIdx) {
                // Угол наклона по вертикали (от 0 на экваторе до PI/2 на полюсе полусферы)
                float angleZ = (static_cast<float>(ringIdx) / static_cast<float>(kSegmentsZ)) * (M_PI * 0.5f);

                // Коэффициент сжатия кольца к центру (по косинусу)
                float radiusScale = std::cos(angleZ);
                // Высота текущего яруса (по синусу)
                float currentHeight = static_cast<float>(hMax) + domeHeight * std::sin(angleZ);

                // Генерируем вершины текущего горизонтального кольца, масштабируя оригинальную форму здания
                for (size_t i = outerRing.first; i <= endIdx; ++i) {
                    float origX = static_cast<float>(vertices[i].x);
                    float origY = static_cast<float>(vertices[i].y);

                    // Плавно стягиваем координаты к геометрическому центру здания
                    float domeX = center.x + (origX - center.x) * radiusScale;
                    float domeY = center.y + (origY - center.y) * radiusScale;

                    totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                        static_cast<int16_t>(domeX),
                        static_cast<int16_t>(extent - domeY), // Переворот по Y, как у вас
                        static_cast<int16_t>(currentHeight),
                        static_cast<int16_t>(1)
                    });
                    totalColors.emplace_back(roofColor);
                }
            }

            // Добавляем финальную ОДНУ вершину — самый пик купола (макушку)
            const uint32_t peakIdx = static_cast<uint32_t>(totalVerices.size()) - domeStartGlobalIdx;
            totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(center.x),
                static_cast<int16_t>(extent - center.y),
                static_cast<int16_t>(hRidge),
                static_cast<int16_t>(1)
            });
            totalColors.emplace_back(roofColor);

            // 5. ГЕНЕРАЦИЯ ИНДЕКСОВ (ТРИАНГУЛЯЦИЯ СЕТКИ КУПОЛА)
            
            // Шаг 5.1: Строим четырехугольные пояса (Quad Strips) между кольцами купола
            for (size_t ringIdx = 0; ringIdx < kSegmentsZ - 1; ++ringIdx) {
                uint32_t thisRingStart = static_cast<uint32_t>(ringIdx * numBaseVertices);
                uint32_t nextRingStart = static_cast<uint32_t>((ringIdx + 1) * numBaseVertices);

                for (size_t i = 0; i < numBaseVertices; ++i) {
                    size_t nextVert = (i + 1 == numBaseVertices) ? 0 : i + 1;

                    uint32_t bCurr = domeStartGlobalIdx + thisRingStart + static_cast<uint32_t>(i);        // Низ-лево
                    uint32_t bNext = domeStartGlobalIdx + thisRingStart + static_cast<uint32_t>(nextVert);    // Низ-право
                    uint32_t tCurr = domeStartGlobalIdx + nextRingStart + static_cast<uint32_t>(i);        // Верх-лево
                    uint32_t tNext = domeStartGlobalIdx + nextRingStart + static_cast<uint32_t>(nextVert);    // Верх-право

                    // Треугольник 1
                    totalIndices.emplace_back(bCurr);
                    totalIndices.emplace_back(bNext);
                    totalIndices.emplace_back(tCurr);

                    // Треугольник 2
                    totalIndices.emplace_back(tCurr);
                    totalIndices.emplace_back(bNext);
                    totalIndices.emplace_back(tNext);
                }
            }

            // Шаг 5.2: Закрываем макушку купола треугольным веером (Triangle Fan), сходящимся в пик
            uint32_t lastRingStart = static_cast<uint32_t>((kSegmentsZ - 1) * numBaseVertices);
            uint32_t globalPeakIdx = domeStartGlobalIdx + peakIdx;

            for (size_t i = 0; i < numBaseVertices; ++i) {
                size_t nextVert = (i + 1 == numBaseVertices) ? 0 : i + 1;

                uint32_t rCurr = domeStartGlobalIdx + lastRingStart + static_cast<uint32_t>(i);
                uint32_t rNext = domeStartGlobalIdx + lastRingStart + static_cast<uint32_t>(nextVert);

                totalIndices.emplace_back(rCurr);
                totalIndices.emplace_back(rNext);
                totalIndices.emplace_back(globalPeakIdx);
            }
        }
        break;

        // case RoofType::eMany: { // strange!
        //     if (rings.empty() || indices.empty()) break;

        //     // 1. ВЫЧИСЛЕНИЕ ВЫСОТ
        //     const float kMinComplexRoofHeightInMeters = std::min(maxHeight / 5.0f, 6.0f);
        //     const auto hRidge = hMax + metersToTileUnits(
        //         (roofHeight == 0.0f ? kMinComplexRoofHeightInMeters : roofHeight),
        //         latlon.x, tileId.z, extentf);

        //     const float roofHeightDelta = static_cast<float>(hRidge - hMax);

        //     // Внешний контур для анализа масштабирования
        //     const auto& outerRing = rings[0];
        //     bool isClosedWithDuplicate = (vertices[outerRing.first].x == vertices[outerRing.second].x && 
        //                                 vertices[outerRing.first].y == vertices[outerRing.second].y);
        //     size_t endIdx = isClosedWithDuplicate ? (outerRing.second - 1u) : outerRing.second;

        //     // 2. ИСПОЛЬЗУЕМ СУЩЕСТВУЮЩУЮ ТРИАНГУЛЯЦИЮ ФУНДАМЕНТА (indices)
        //     // Каждая тройка индексов из массива indices — это один треугольник основания.
        //     // Мы превратим каждый такой треугольник в 3D-пирамиду (3 боковых ската).
        //     for (size_t t = 0; t < indices.size(); t += 3) {
        //         if (t + 2 >= indices.size()) break;

        //         // Индексы трех вершин текущего треугольника основания
        //         uint32_t idx0 = indices[t];
        //         uint32_t idx1 = indices[t + 1];
        //         uint32_t idx2 = indices[t + 2];

        //         const auto& v0 = vertices[idx0];
        //         const auto& v1 = vertices[idx1];
        //         const auto& v2 = vertices[idx2];

        //         // Находим центроид (геометрический центр) текущего треугольника
        //         float cX = (static_cast<float>(v0.x) + static_cast<float>(v1.x) + static_cast<float>(v2.x)) / 3.0f;
        //         float cY = (static_cast<float>(v0.y) + static_cast<float>(v1.y) + static_cast<float>(v2.y)) / 3.0f;

        //         // ВАЖНО: Вычисляем высоту пика этой конкретной мини-пирамиды.
        //         // Чтобы крыша не была плоской, высота пика должна зависеть от размера треугольника 
        //         // (у узких крыльев здания пики будут ниже, у широких — выше, создавая честный рельеф).
        //         float len0 = glm::distance(engine::vec2f{(float)v0.x, (float)v0.y}, engine::vec2f{(float)v1.x, (float)v1.y});
        //         float len1 = glm::distance(engine::vec2f{(float)v1.x, (float)v1.y}, engine::vec2f{(float)v2.x, (float)v2.y});
        //         float len2 = glm::distance(engine::vec2f{(float)v2.x, (float)v2.y}, engine::vec2f{(float)v0.x, (float)v0.y});
        //         float maxEdge = std::max({len0, len1, len2});

        //         // Высота пропорциональна размеру треугольника, но не превышает общий лимит hRidge
        //         float currentPeakHeight = static_cast<float>(hMax) + std::min(roofHeightDelta, maxEdge * 0.4f);

        //         // 3. ЗАПИСЫВАЕМ ВЕРШИНЫ ДЛЯ ТЕКУЩЕЙ ПИРАМИДЫ СКАТА
        //         // Чтобы избежать проблем со сглаживанием нормалей (hard edges), генерируем вершины изолированно
        //         uint32_t startIdx = static_cast<uint32_t>(totalVerices.size());

        //         // Точки основания (на высоте hMax)
        //         totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{(int16_t)v0.x, (int16_t)(extent - v0.y), (int16_t)hMax, 1});
        //         totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{(int16_t)v1.x, (int16_t)(extent - v1.y), (int16_t)hMax, 1});
        //         totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{(int16_t)v2.x, (int16_t)(extent - v2.y), (int16_t)hMax, 1});

        //         // Вершина пика (конек / макушка пирамиды)
        //         totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
        //             static_cast<int16_t>(std::round(cX)),
        //             static_cast<int16_t>(std::round(extent - cY)),
        //             static_cast<int16_t>(std::round(currentPeakHeight)),
        //             1
        //         });

        //         for (int k = 0; k < 4; ++k) totalColors.emplace_back(roofColor);

        //         // Local индексы: 0, 1, 2 — основание, 3 — пик пирамиды
        //         // 4. СТРОИМ 3 БОКОВЫХ ТРЕУГОЛЬНИКА ПИРАМИДЫ
        //         // Скат 1 (над ребром 0-1)
        //         totalIndices.emplace_back(startIdx + 0);
        //         totalIndices.emplace_back(startIdx + 1);
        //         totalIndices.emplace_back(startIdx + 3);

        //         // Скат 2 (над ребром 1-2)
        //         totalIndices.emplace_back(startIdx + 1);
        //         totalIndices.emplace_back(startIdx + 2);
        //         totalIndices.emplace_back(startIdx + 3);

        //         // Скат 3 (над ребром 2-0)
        //         totalIndices.emplace_back(startIdx + 2);
        //         totalIndices.emplace_back(startIdx + 0);
        //         totalIndices.emplace_back(startIdx + 3);
        //     }
        // }
        // break;

        case RoofType::eSkillion: { // superb!
            if (rings.empty() || indices.empty() || vertices.empty()) break;

            // 1. ВЫЧИСЛЕНИЕ МАКСИМАЛЬНОЙ ВЫСОТЫ (ПИКА НАКЛОНА)
            const float kMinSkillionHeightInMeters = std::min(maxHeight / 6.0f, 3.5f);
            const auto hRidge = hMax + metersToTileUnits(
                (roofHeight == 0.0f ? kMinSkillionHeightInMeters : roofHeight),
                latlon.x, tileId.z, extentf);
            
            const float roofHeightDelta = static_cast<float>(hRidge - hMax);

            // 2. ОПРЕДЕЛЯЕМ НАПРАВЛЕНИЕ НАКЛОНА КРЫШИ
            // Ищем самую длинную стену здания по ВСЕМ кольцам, чтобы точнее определить фасад
            engine::vec2f mainAxis{1.0f, 0.0f};
            float maxWallLenSq = 0.0f;

            for (const auto& ring : rings) {
                if (ring.second < ring.first) continue;
                bool isClosed = (vertices[ring.first].x == vertices[ring.second].x && 
                                vertices[ring.first].y == vertices[ring.second].y);
                size_t endIdx = isClosed ? (ring.second - 1u) : ring.second;
                size_t count = endIdx - ring.first + 1;

                for (size_t i = 0; i < count; ++i) {
                    size_t curr = ring.first + i;
                    size_t next = (i == count - 1) ? ring.first : (curr + 1u);
                    
                    engine::vec2f wall{static_cast<float>(vertices[next].x - vertices[curr].x), 
                                    static_cast<float>(vertices[next].y - vertices[curr].y)};
                    float lenSq = wall.x * wall.x + wall.y * wall.y;
                    if (lenSq > maxWallLenSq) {
                        maxWallLenSq = lenSq;
                        mainAxis = glm::normalize(wall);
                    }
                }
            }

            // Вектор наклона (перпендикуляр к самой длинной стене)
            engine::vec2f slopeDirection{-mainAxis.y, mainAxis.x};

            // Находим глобальные границы проекции абсолютно для всех вершин здания
            float minProj = std::numeric_limits<float>::max();
            float maxProj = std::numeric_limits<float>::lowest();

            for (const auto& vtx : vertices) {
                engine::vec2f v{static_cast<float>(vtx.x), static_cast<float>(vtx.y)};
                float proj = glm::dot(v, slopeDirection);
                if (proj < minProj) minProj = proj;
                if (proj > maxProj) maxProj = proj;
            }

            float projLength = maxProj - minProj;
            if (projLength < 1.0f) projLength = 1.0f;

            // ==============================================================================
            // ПОЛИГОН 1: НИЖНИЙ ПОЛИГОН КРЫШИ (Монолитное плоское основание на высоте hMax)
            const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            for (const auto& vtx : vertices) {
                totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                    static_cast<int16_t>(vtx.x), 
                    static_cast<int16_t>(extent - vtx.y), 
                    static_cast<int16_t>(hMax), 1
                });
                totalColors.emplace_back(roofColor);
            }

            // Отрисовка нижней крышки (инвертированная, как у вас)
            for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
                totalIndices.emplace_back(lowerPolygonStartIdx + *it);
            }

            // ==============================================================================
            // ПОЛИГОН 2: ВЕРХНИЙ ПОЛИГОН КРЫШИ (Полный дубликат всех вершин, но с наклоном)
            const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            for (const auto& vtx : vertices) {
                engine::vec2f v{static_cast<float>(vtx.x), static_cast<float>(vtx.y)};
                float proj = glm::dot(v, slopeDirection);
                
                float t = (proj - minProj) / projLength;
                t = std::max(0.0f, std::min(1.0f, t));

                float calculatedHeight = static_cast<float>(hMax) + roofHeightDelta * t;

                totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                    static_cast<int16_t>(vtx.x), 
                    static_cast<int16_t>(extent - vtx.y), 
                    static_cast<int16_t>(std::round(calculatedHeight)), 1
                });
                totalColors.emplace_back(roofColor);
            }

            // Исходная триангуляция фундамента теперь идеально покроет наклонный топ, включая все дыры
            for (auto idx : indices) {
                totalIndices.emplace_back(upperPolygonStartIdx + idx);
            }

            // ==============================================================================
            // ШАГ 3: СТРОИМ СТЕНЫ КРЫШИ (Генерируем вертикальные юбки для каждого кольца отдельно)
            // Проходим циклом по ВСЕМ кольцам (внешнему и внутренним дворам), чтобы связать их стены
            for (const auto& ring : rings) {
                if (ring.second < ring.first) continue;

                bool isClosed = (vertices[ring.first].x == vertices[ring.second].x && 
                                vertices[ring.first].y == vertices[ring.second].y);
                size_t endIdxLocal = isClosed ? (ring.second - 1u) : ring.second;

                for (size_t i = ring.first; i <= endIdxLocal; ++i) {
                    const auto nextIdx = (i == endIdxLocal) ? ring.first : (i + 1u);

                    const auto eCurrent = lowerPolygonStartIdx + static_cast<uint32_t>(i);
                    const auto eNext    = lowerPolygonStartIdx + static_cast<uint32_t>(nextIdx);
                    
                    const auto rCurrent = upperPolygonStartIdx + static_cast<uint32_t>(i);
                    const auto rNext    = upperPolygonStartIdx + static_cast<uint32_t>(nextIdx);

                    // Боковые стенки для текущего сегмента кольца
                    totalIndices.emplace_back(eCurrent);
                    totalIndices.emplace_back(eNext);
                    totalIndices.emplace_back(rCurrent);

                    totalIndices.emplace_back(rCurrent);
                    totalIndices.emplace_back(eNext);
                    totalIndices.emplace_back(rNext);
                }
            }
        }
        break;

        case RoofType::ePlants: { // good?
            const float kMinPlantsRoofHeightInMeters = 0.5f;
            const auto hRidge = hMax + metersToTileUnits(
                (roofHeight == 0.0f ? kMinPlantsRoofHeightInMeters : roofHeight),
                latlon.x, tileId.z, extentf);
            
            // Фиксированный отступ ската в единицах тайла
            const float insetDistance = metersToTileUnits(0.5f, latlon.x, tileId.z, extentf);

            // ==============================================================================
            // ПОЛИГОН 1: НИЖНИЙ ПОЛИГОН КРЫШИ (Карниз на высоте hMax)
            const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            // roof bottom plane
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

            // roof bottom plane indices
            for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
                totalIndices.emplace_back(lowerPolygonStartIdx + *it);
            }

            // ==============================================================================
            // ПОЛИГОН 2: ВЕРХНИЙ ПОЛИГОН КРЫШИ (Конёк на высоте hRidge, суженный по биссектрисам)
            const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            // Изначально заполняем верхний полигон чистыми копиями для сохранения структуры индексов Шага 2
            for (auto vtx : vertices) {
                totalVerices.emplace_back(
                    engine::stl::array<int16_t, 4u>{
                        static_cast<int16_t>(vtx.x), 
                        static_cast<int16_t>(extent - vtx.y), 
                        static_cast<int16_t>(hRidge), 
                        static_cast<int16_t>(1)
                    }
                );
                totalColors.emplace_back(roofColor);
            }

            // Создаем структуру для хранения новой геометрии конька под повторный Earcut
            // Формат зависит от вашего типа Point, предположим std::vector<std::vector<PointType>>
            // где первый элемент — внешнее кольцо, последующие — дыры.
            using Point2D = engine::vec2f; // Подставьте ваш тип точки, если нужен int/float
            std::vector<std::vector<Point2D>> earcutPolygonData;
            earcutPolygonData.reserve(rings.size());

            // Модифицируем вершины верхнего полигона по вашему лаконичному алгоритму
            for (const auto & ring : rings) {
                if (ring.second < ring.first) continue;

                bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                              vertices[ring.first].y == vertices[ring.second].y);
                size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;

                // Создаем локальное кольцо для триангулятора
                std::vector<Point2D> localNewRing;
                localNewRing.reserve((ring.second - ring.first) + 1);

                for (size_t i = ring.first; i <= ring.second; ++i) {
                    size_t prevIdx, nextIdx;
                    if (isClosedWithDuplicate && i == ring.second) {
                        prevIdx = endIdx;
                        nextIdx = ring.first + 1u;
                    } else {
                        prevIdx = (i == ring.first) ? endIdx : (i - 1u);
                        nextIdx = (i == endIdx) ? ring.first : (i + 1u);
                    }

                    const auto & vCurr = vertices[i];
                    const auto & vPrev = vertices[prevIdx];
                    const auto & vNext = vertices[nextIdx];

                    const engine::vec2f d1{vCurr.x - vPrev.x, vCurr.y - vPrev.y};
                    const engine::vec2f d2{vNext.x - vCurr.x, vNext.y - vCurr.y};

                    const engine::vec2f n1 = glm::normalize(engine::vec2f{-d1.y, d1.x});
                    const engine::vec2f n2 = glm::normalize(engine::vec2f{-d2.y, d2.x});

                    const engine::vec2f shiftDir = glm::normalize(n1 + n2);

                    const float dotProd = glm::dot(shiftDir, n1);
                    float length = insetDistance;
                    
                    if (std::abs(dotProd) > 0.1f) {
                        length = insetDistance / std::abs(dotProd);
                    }

                    if (length > insetDistance * 1.8f) {
                        length = insetDistance * 1.8f;
                    }

                    const float ridgePtX = static_cast<float>(vCurr.x) + shiftDir.x * length;
                    const float ridgePtY = static_cast<float>(vCurr.y) + shiftDir.y * length;

                    const int16_t finalX = static_cast<int16_t>(std::round(ridgePtX));
                    const int16_t finalY = static_cast<int16_t>(std::round(ridgePtY));

                    // Пишем сдвинутые координаты в глобальный буфер Полигона 2
                    totalVerices[upperPolygonStartIdx + i] = engine::stl::array<int16_t, 4u>{
                        finalX, 
                        static_cast<int16_t>(extent - finalY), 
                        static_cast<int16_t>(hRidge), 
                        static_cast<int16_t>(1)
                    };

                    // Сохраняем точку для повторного Earcut (в исходной системе координат)
                    localNewRing.emplace_back(Point2D{ridgePtX, ridgePtY});
                }

                earcutPolygonData.emplace_back(std::move(localNewRing));
            }

            // ==============================================================================
            // ШАГ 2: СТРОИМ СТЕНЫ КРЫШИ (Боковые покатые наклонные грани)
            // Строятся по локальным rings, связывая нижний и верхний монолитные буферы
            for (const auto & ring : rings) {
                if (ring.second < ring.first) continue;

                bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                              vertices[ring.first].y == vertices[ring.second].y);
                size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;

                for (size_t i = ring.first; i <= endIdx; ++i) {
                    const auto nextIdx = (i == endIdx) ? ring.first : (i + 1u);

                    const auto eCurrent = lowerPolygonStartIdx + static_cast<uint32_t>(i);
                    const auto eNext    = lowerPolygonStartIdx + static_cast<uint32_t>(nextIdx);
                    
                    const auto rCurrent = upperPolygonStartIdx + static_cast<uint32_t>(i);
                    const auto rNext    = upperPolygonStartIdx + static_cast<uint32_t>(nextIdx);

                    totalIndices.emplace_back(eCurrent);
                    totalIndices.emplace_back(eNext);
                    totalIndices.emplace_back(rCurrent);

                    totalIndices.emplace_back(rCurrent);
                    totalIndices.emplace_back(eNext);
                    totalIndices.emplace_back(rNext);
                }
            }

            // ==============================================================================
            // ШАГ 3: ПОВТОРНАЯ ТРИАНГУЛЯЦИЯ СЖАТЫХ ВЕРШИН
            // Передаем новый суженный контур здания в Earcut. 
            // Он построит чистые треугольники без вылетов во двор и без учета старой топологии.
            // Примечание: Подставьте точный вызов вашего Earcut-декодера, если интерфейс отличается.
            std::vector<uint32_t> newRidgeIndices = mapbox::earcut<uint32_t>(earcutPolygonData);

            for (auto idx : newRidgeIndices) {
                if (idx < vertices.size()) {
                    totalIndices.emplace_back(upperPolygonStartIdx + idx);
                }
            }
        }
        break;

        case RoofType::ePyramidal: {
            if (rings.empty() || vertices.empty()) break;

            // 1. ВЫЧИСЛЕНИЕ ВЫСОТЫ ПИКА
            const float kMinPyramidHeightInMeters = 4.0f;
            const auto hRidge = hMax + metersToTileUnits(
                (roofHeight == 0.0f ? kMinPyramidHeightInMeters : roofHeight),
                latlon.x, tileId.z, extentf);

            const auto& ring = rings[0u]; // Работаем с внешним контуром
            bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                        vertices[ring.first].y == vertices[ring.second].y);
            size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;
            size_t count = endIdx - ring.first + 1;

            if (count < 3) break;

            // ==============================================================================
            // ШАГ 1: СТРОИМ НИЖНЕЕ ОСНОВАНИЕ КРЫШИ (Карниз на высоте hMax)
            const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            for (size_t i = ring.first; i <= ring.second; ++i) {
                totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                    static_cast<int16_t>(vertices[i].x), 
                    static_cast<int16_t>(extent - vertices[i].y), 
                    static_cast<int16_t>(hMax), 1
                });
                totalColors.emplace_back(roofColor);
            }

            // Заглушка пола крыши (реверсивные индексы)
            for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
                totalIndices.emplace_back(lowerPolygonStartIdx + *it);
            }

            // ==============================================================================
            // ШАГ 2: ВЫЧИСЛЯЕМ ЦЕНТР И СОЗДАЕМ ПИК ПИРАМИДЫ (на высоте hRidge)
            engine::vec2f center{0.0f, 0.0f};
            for (size_t i = ring.first; i <= endIdx; ++i) {
                center.x += static_cast<float>(vertices[i].x);
                center.y += static_cast<float>(vertices[i].y);
            }
            center.x /= static_cast<float>(count);
            center.y /= static_cast<float>(count);

            // Добавляем одну единственную вершину вершины-пика в глобальный буфер
            const uint32_t peakGlobalIdx = static_cast<uint32_t>(totalVerices.size());
            totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(center.x),
                static_cast<int16_t>(extent - center.y),
                static_cast<int16_t>(hRidge),
                1
            });
            totalColors.emplace_back(roofColor);

            // ==============================================================================
            // ШАГ 3: ТРИАНГУЛИРУЕМ СКА ТЫ (Соединяем ребра карниза с пиком пирамиды)
            // Проходим по всем ребрам основания и строим треугольные скаты к вершине
            for (size_t i = 0; i < count; ++i) {
                const auto nextIdx = (i == count - 1) ? 0 : (i + 1);

                const auto eCurrent = lowerPolygonStartIdx + static_cast<uint32_t>(i);
                const auto eNext    = lowerPolygonStartIdx + static_cast<uint32_t>(nextIdx);

                // Порядок индексов оптимизирован под стандартный Backface Culling (лицом наружу)
                totalIndices.emplace_back(peakGlobalIdx);
                totalIndices.emplace_back(eCurrent);
                totalIndices.emplace_back(eNext);
            }
        }
        break;

        default:
        {
            // ПОЛИГОН 2: ВЕРХНИЙ ПОЛИГОН КРЫШИ (Конёк на высоте hRidge, суженный по биссектрисам)
            const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());

            // Изначально заполняем верхний полигон чистыми копиями для сохранения структуры индексов Шага 2
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

    static constexpr MaterialEntry kPolyginMaterials[] = {
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
                    std::begin(kPolyginMaterials),
                    std::end(kPolyginMaterials),
                    [&value] (const MaterialEntry & material) noexcept {
                        return value == material.name;
                    }
                );

                if (it != std::end(kPolyginMaterials)) {
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

                if (value.starts_with('#')) {
                    color = hexStringToUint32(value).color;
                } else {
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
                        LOG_W("no found color uint32_t for : %s", value.c_str());
                    }
                }
            }

            if (str == "building_material") {
                const auto value = property.value().string_value().to_string();

                const auto it = std::find_if(
                    std::begin(kBuildingMaterials),
                    std::end(kBuildingMaterials),
                    [&value] (const MaterialEntry & material) noexcept {
                        return value == material.name;
                    }
                );

                if (it != std::end(kBuildingMaterials)) {
                    materialIdx = it->id;
                } else {
                    LOG_W("no found material uint32_t for : %s", value.c_str());
                }
            }

            if (str == "roof_colour") {
                const auto value = property.value().string_value().to_string();

                if (value.starts_with('#')) {
                    roofColor = hexStringToUint32(value).color;
                } else {
                    const auto delimeter = std::min(value.find(';'), value.find('\''));
                    bool colorFoundResult = false;
                    if (delimeter != std::string::npos) { // get first color
                        const auto [c, result] = colorNameToUint32t({value.begin(), value.begin() + delimeter});
                        //const auto [c, result] = colorNameToUint32t({str.begin() + delimeter + 1u, str.end()});
                        roofColor = c;
                        colorFoundResult = result;
                    } else {
                        const auto [c, result] = colorNameToUint32t(value);
                        roofColor = c;
                        colorFoundResult = result;
                    }

                    if (!colorFoundResult) {
                        LOG_W("no found roofColor uint32_t for : %s", value.c_str());
                    }
                }
            }

            if (str == "roof_shape") {
                const auto value = property.value().string_value().to_string();
                roofType = parseRoofType(value);

                // switch (roofType) {
                // case RoofType::eMany:
                // case RoofType::eMansard:
                // case RoofType::eRound:
                // case RoofType::eGabled:
                // case RoofType::eHipped:
                //     materialIdx2 = 12;
                //     break;
                // default:
                //     break;
                // }
            }

            if (str == "roof_height") {
                roofHeight = getNumericValue<float>(property.value());
            }

            if (str == "roof_material") {
                const auto value = "roof_" + property.value().string_value().to_string();

                const auto it = std::find_if(
                    std::begin(kBuildingMaterials),
                    std::end(kBuildingMaterials),
                    [&value] (const MaterialEntry & material) noexcept {
                        return value == material.name;
                    }
                );

                if (it != std::end(kBuildingMaterials)) {
                    materialIdx2 = it->id;
                } else {
                    LOG_W("no found roof material uint32_t for : %s", value.c_str());
                }
            }

            if (str == "windows") {
                const auto value = property.value().string_value().to_string();
                LOG_W("windows key found, value : %s", value.c_str());
            }

            // if (str == "kind") {
            //     auto value = stringify(property.value());
            //     //printf("kind: %s\n", value.c_str());
            //     if (value == "building") {
            //         materialIdx = 0;//engine::common::fixedRandom(0, 1);
            //     } else if (value == "grass" || value == "park" || value == "grassland") {
            //         materialIdx = 5;
            //     } else if (value == "pedestrian") {
            //         materialIdx = 6;
            //     } else if (value == "wood" || value == "scrub") {
            //         materialIdx = 7;
            //     } else if (value == "industrial") {
            //         materialIdx = 8;
            //     } else if (value == "water") {
            //         materialIdx = 9;
            //     }
            // } else {
            //     materialIdx = 1;//engine::common::fixedRandom(0, 1);
            // }
            // if (str == "kind_detail") {
            //     auto value = stringify(property.value());
            //     int a = 0;
            //     printf("kind_detail: %s\n", stringify(property.value()).c_str());
            // }
            //printf("%s = %s\n", property.key().to_string().c_str(), stringify(property.value()).c_str());
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