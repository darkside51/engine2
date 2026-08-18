#include "roofs.h"
#include "map/geography.h"

#include "engine/src/math/earcut.h"

namespace map {
    
void buildSkillionRoof(
    std::span<uint32_t> indices, 
    std::span<engine::vec2i> vertices, 
    std::span<std::pair<uint32_t, uint32_t>> rings,
    engine::stl::vector<uint32_t> & totalIndices,
    engine::stl::vector<engine::stl::array<int16_t, 4u>> & totalVerices,
    engine::stl::vector<uint32_t> & totalColors,
    float maxHeight,
    float roofHeight,
    float hMax,
    engine::vec2f latlon,
    float extentf,
    uint32_t roofColor,
    uint8_t z
    ) { // superb!
    const float kMinSkillionHeightInMeters = std::min(maxHeight / 6.0f, 3.5f);
    const auto hRidge = hMax + metersToTileUnits(
        (roofHeight == 0.0f ? kMinSkillionHeightInMeters : roofHeight),
        latlon.x, z, extentf);
    
    const float roofHeightDelta = static_cast<float>(hRidge - hMax);

    // find the longest wall of the building along ALL rings to more accurately determine the façade
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

    // slope vector (perpendicular to the longest wall)
    engine::vec2f slopeDirection{-mainAxis.y, mainAxis.x};

    // find the global projection boundaries for absolutely all building vertices
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


    const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    for (const auto& vtx : vertices) {
        totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
            static_cast<int16_t>(vtx.x), 
            static_cast<int16_t>(extentf - vtx.y), 
            static_cast<int16_t>(hMax), 1
        });
        totalColors.emplace_back(roofColor);
    }

    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
        totalIndices.emplace_back(lowerPolygonStartIdx + *it);
    }

    const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    for (const auto& vtx : vertices) {
        engine::vec2f v{static_cast<float>(vtx.x), static_cast<float>(vtx.y)};
        float proj = glm::dot(v, slopeDirection);
        
        float t = (proj - minProj) / projLength;
        t = std::max(0.0f, std::min(1.0f, t));

        float calculatedHeight = static_cast<float>(hMax) + roofHeightDelta * t;

        totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
            static_cast<int16_t>(vtx.x), 
            static_cast<int16_t>(extentf - vtx.y), 
            static_cast<int16_t>(std::round(calculatedHeight)), 1
        });
        totalColors.emplace_back(roofColor);
    }

    for (auto idx : indices) {
        totalIndices.emplace_back(upperPolygonStartIdx + idx);
    }

    // faces
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

            totalIndices.emplace_back(eCurrent);
            totalIndices.emplace_back(eNext);
            totalIndices.emplace_back(rCurrent);

            totalIndices.emplace_back(rCurrent);
            totalIndices.emplace_back(eNext);
            totalIndices.emplace_back(rNext);
        }
    }
}

void buildDomeRoof(
    std::span<uint32_t> indices, 
    std::span<engine::vec2i> vertices, 
    std::span<std::pair<uint32_t, uint32_t>> rings,
    engine::stl::vector<uint32_t> & totalIndices,
    engine::stl::vector<engine::stl::array<int16_t, 4u>> & totalVerices,
    engine::stl::vector<uint32_t> & totalColors,
    float maxHeight,
    float roofHeight,
    float hMax,
    engine::vec2f latlon,
    float extentf,
    uint32_t roofColor,
    uint8_t z
) {
    const float kMinDomeHeightInMeters = 4.0f;
    const auto hRidge = hMax + metersToTileUnits(
        (roofHeight == 0.0f ? kMinDomeHeightInMeters : roofHeight),
        latlon.x, z, extentf);
    
    const float domeHeight = static_cast<float>(hRidge - hMax);

    const auto& outerRing = rings[0u]; 
    bool isClosedWithDuplicate = (vertices[outerRing.first].x == vertices[outerRing.second].x && 
                                vertices[outerRing.first].y == vertices[outerRing.second].y);
    size_t endIdx = isClosedWithDuplicate ? (outerRing.second - 1u) : outerRing.second;
    size_t numBaseVertices = endIdx - outerRing.first + 1;

    if (numBaseVertices < 3u) {
        return;
    }

    // get geometry center
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

    // dome detalisation
    constexpr size_t kSegmentsZ = 8; 

    const uint32_t domeStartGlobalIdx = static_cast<uint32_t>(totalVerices.size());

    // from bottom to top
    for (size_t ringIdx = 0; ringIdx < kSegmentsZ; ++ringIdx) {
        float angleZ = (static_cast<float>(ringIdx) / static_cast<float>(kSegmentsZ)) * (M_PI * 0.5f);
        float radiusScale = std::cos(angleZ);
        float currentHeight = static_cast<float>(hMax) + domeHeight * std::sin(angleZ);

        for (size_t i = outerRing.first; i <= endIdx; ++i) {
            float origX = static_cast<float>(vertices[i].x);
            float origY = static_cast<float>(vertices[i].y);

            float domeX = center.x + (origX - center.x) * radiusScale;
            float domeY = center.y + (origY - center.y) * radiusScale;

            totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(domeX),
                static_cast<int16_t>(extentf - domeY),
                static_cast<int16_t>(currentHeight),
                static_cast<int16_t>(1)
            });
            totalColors.emplace_back(roofColor);
        }
    }

    const uint32_t peakIdx = static_cast<uint32_t>(totalVerices.size()) - domeStartGlobalIdx;
    totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
        static_cast<int16_t>(center.x),
        static_cast<int16_t>(extentf - center.y),
        static_cast<int16_t>(hRidge),
        static_cast<int16_t>(1)
    });
    totalColors.emplace_back(roofColor);

    for (size_t ringIdx = 0; ringIdx < kSegmentsZ - 1; ++ringIdx) {
        uint32_t thisRingStart = static_cast<uint32_t>(ringIdx * numBaseVertices);
        uint32_t nextRingStart = static_cast<uint32_t>((ringIdx + 1) * numBaseVertices);

        for (size_t i = 0; i < numBaseVertices; ++i) {
            size_t nextVert = (i + 1 == numBaseVertices) ? 0 : i + 1;

            uint32_t bCurr = domeStartGlobalIdx + thisRingStart + static_cast<uint32_t>(i);        // Низ-лево
            uint32_t bNext = domeStartGlobalIdx + thisRingStart + static_cast<uint32_t>(nextVert);    // Низ-право
            uint32_t tCurr = domeStartGlobalIdx + nextRingStart + static_cast<uint32_t>(i);        // Верх-лево
            uint32_t tNext = domeStartGlobalIdx + nextRingStart + static_cast<uint32_t>(nextVert);    // Верх-право

            totalIndices.emplace_back(bCurr);
            totalIndices.emplace_back(bNext);
            totalIndices.emplace_back(tCurr);

            totalIndices.emplace_back(tCurr);
            totalIndices.emplace_back(bNext);
            totalIndices.emplace_back(tNext);
        }
    }

    uint32_t lastRingStart = static_cast<uint32_t>((kSegmentsZ - 1u) * numBaseVertices);
    uint32_t globalPeakIdx = domeStartGlobalIdx + peakIdx;

    for (size_t i = 0u; i < numBaseVertices; ++i) {
        size_t nextVert = (i + 1u == numBaseVertices) ? 0u : i + 1u;

        uint32_t rCurr = domeStartGlobalIdx + lastRingStart + static_cast<uint32_t>(i);
        uint32_t rNext = domeStartGlobalIdx + lastRingStart + static_cast<uint32_t>(nextVert);

        totalIndices.emplace_back(rCurr);
        totalIndices.emplace_back(rNext);
        totalIndices.emplace_back(globalPeakIdx);
    }
}

void buildPyramidalRoof(
    std::span<uint32_t> indices, 
    std::span<engine::vec2i> vertices, 
    std::span<std::pair<uint32_t, uint32_t>> rings,
    engine::stl::vector<uint32_t> & totalIndices,
    engine::stl::vector<engine::stl::array<int16_t, 4u>> & totalVerices,
    engine::stl::vector<uint32_t> & totalColors,
    float maxHeight,
    float roofHeight,
    float hMax,
    engine::vec2f latlon,
    float extentf,
    uint32_t roofColor,
    uint8_t z
) {
    const float kMinPyramidHeightInMeters = 4.0f;
    const auto hRidge = hMax + metersToTileUnits(
        (roofHeight == 0.0f ? kMinPyramidHeightInMeters : roofHeight),
        latlon.x, z, extentf);

    const auto& ring = rings[0u]; // Работаем с внешним контуром
    bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                vertices[ring.first].y == vertices[ring.second].y);
    size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;
    size_t count = endIdx - ring.first + 1;

    if (count < 3u) {
        return;
    }

    const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    for (size_t i = ring.first; i <= ring.second; ++i) {
        totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
            static_cast<int16_t>(vertices[i].x), 
            static_cast<int16_t>(extentf - vertices[i].y), 
            static_cast<int16_t>(hMax), 1
        });
        totalColors.emplace_back(roofColor);
    }

    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
        totalIndices.emplace_back(lowerPolygonStartIdx + *it);
    }

    // calculate center
    engine::vec2f center{0.0f, 0.0f};
    for (size_t i = ring.first; i <= endIdx; ++i) {
        center.x += static_cast<float>(vertices[i].x);
        center.y += static_cast<float>(vertices[i].y);
    }
    center.x /= static_cast<float>(count);
    center.y /= static_cast<float>(count);

    // peak vertex
    const uint32_t peakGlobalIdx = static_cast<uint32_t>(totalVerices.size());
    totalVerices.emplace_back(engine::stl::array<int16_t, 4u>{
        static_cast<int16_t>(center.x),
        static_cast<int16_t>(extentf - center.y),
        static_cast<int16_t>(hRidge),
        1
    });
    totalColors.emplace_back(roofColor);

    // faces
    for (size_t i = 0u; i < count; ++i) {
        const auto nextIdx = (i == count - 1u) ? 0u : (i + 1u);

        const auto eCurrent = lowerPolygonStartIdx + static_cast<uint32_t>(i);
        const auto eNext    = lowerPolygonStartIdx + static_cast<uint32_t>(nextIdx);

        totalIndices.emplace_back(peakGlobalIdx);
        totalIndices.emplace_back(eCurrent);
        totalIndices.emplace_back(eNext);
    }
}

void buildPlantsRoof(
    std::span<uint32_t> indices, 
    std::span<engine::vec2i> vertices, 
    std::span<std::pair<uint32_t, uint32_t>> rings,
    engine::stl::vector<uint32_t> & totalIndices,
    engine::stl::vector<engine::stl::array<int16_t, 4u>> & totalVerices,
    engine::stl::vector<uint32_t> & totalColors,
    float maxHeight,
    float roofHeight,
    float hMax,
    engine::vec2f latlon,
    float extentf,
    uint32_t roofColor,
    uint8_t z
) {
    const float kMinPlantsRoofHeightInMeters = 0.5f;
    const auto hRidge = hMax + metersToTileUnits(
        (roofHeight == 0.0f ? kMinPlantsRoofHeightInMeters : roofHeight),
        latlon.x, z, extentf);
    
    const float insetDistance = metersToTileUnits(0.5f, latlon.x, z, extentf);


    const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    // roof bottom plane
    for (auto vtx : vertices) {
        totalVerices.emplace_back(
            engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(vtx.x), 
                static_cast<int16_t>(extentf - vtx.y), 
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

    const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    for (auto vtx : vertices) {
        totalVerices.emplace_back(
            engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(vtx.x), 
                static_cast<int16_t>(extentf - vtx.y), 
                static_cast<int16_t>(hRidge), 
                static_cast<int16_t>(1)
            }
        );
        totalColors.emplace_back(roofColor);
    }

    std::vector<std::vector<engine::vec2f>> earcutPolygonData;
    earcutPolygonData.reserve(rings.size());

    for (const auto & ring : rings) {
        if (ring.second < ring.first) continue;

        bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                        vertices[ring.first].y == vertices[ring.second].y);
        size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;

        std::vector<engine::vec2f> localNewRing;
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

            totalVerices[upperPolygonStartIdx + i] = engine::stl::array<int16_t, 4u>{
                finalX, 
                static_cast<int16_t>(extentf - finalY), 
                static_cast<int16_t>(hRidge), 
                static_cast<int16_t>(1)
            };

            localNewRing.emplace_back(engine::vec2f{ridgePtX, ridgePtY});
        }

        earcutPolygonData.emplace_back(std::move(localNewRing));
    }

    // faces
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

    // top face
    std::vector<uint32_t> newRidgeIndices = engine::earcut<uint32_t>(earcutPolygonData);
    for (auto idx : newRidgeIndices) {
        if (idx < vertices.size()) {
            totalIndices.emplace_back(upperPolygonStartIdx + idx);
        }
    }
}

void buildHippedRoof(
    std::span<uint32_t> indices, 
    std::span<engine::vec2i> vertices, 
    std::span<std::pair<uint32_t, uint32_t>> rings,
    engine::stl::vector<uint32_t> & totalIndices,
    engine::stl::vector<engine::stl::array<int16_t, 4u>> & totalVerices,
    engine::stl::vector<uint32_t> & totalColors,
    float maxHeight,
    float roofHeight,
    float hMax,
    engine::vec2f latlon,
    float extentf,
    uint32_t roofColor,
    uint8_t z
) {
    const float kMinHippedRoofHeightInMeters = std::min(maxHeight / 6.0f, 4.0f);
    const auto hRidge = hMax + metersToTileUnits(
        (roofHeight == 0.0f ? kMinHippedRoofHeightInMeters : roofHeight),
        latlon.x, z, extentf);
    
    const float insetDistance = metersToTileUnits(1.5f, latlon.x, z, extentf);
    constexpr float kBottomOffset = 4.0f;

    const uint32_t lowerPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    // roof bottom plane
    for (auto vtx : vertices) {
        totalVerices.emplace_back(
            engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(vtx.x), 
                static_cast<int16_t>(extentf - vtx.y), 
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

    const uint32_t upperPolygonStartIdx = static_cast<uint32_t>(totalVerices.size());
    for (auto vtx : vertices) {
        totalVerices.emplace_back(
            engine::stl::array<int16_t, 4u>{
                static_cast<int16_t>(vtx.x), 
                static_cast<int16_t>(extentf - vtx.y), 
                static_cast<int16_t>(hRidge), 
                static_cast<int16_t>(1)
            }
        );
        totalColors.emplace_back(roofColor);
    }

    std::vector<std::vector<engine::vec2f>> earcutPolygonData;
    earcutPolygonData.reserve(rings.size());

    for (const auto & ring : rings) {
        if (ring.second < ring.first) continue;

        bool isClosedWithDuplicate = (vertices[ring.first].x == vertices[ring.second].x && 
                                        vertices[ring.first].y == vertices[ring.second].y);
        size_t endIdx = isClosedWithDuplicate ? (ring.second - 1u) : ring.second;

        std::vector<engine::vec2f> localNewRing;
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

            totalVerices[upperPolygonStartIdx + i] = engine::stl::array<int16_t, 4u>{
                finalX, 
                static_cast<int16_t>(extentf - finalY), 
                static_cast<int16_t>(hRidge), 
                static_cast<int16_t>(1)
            };

            // enlarge bottom plane
            const float ridgePtX2 = static_cast<float>(vCurr.x) - shiftDir.x * kBottomOffset;
            const float ridgePtY2 = static_cast<float>(vCurr.y) - shiftDir.y * kBottomOffset;

            const int16_t finalX2 = static_cast<int16_t>(std::round(ridgePtX2));
            const int16_t finalY2 = static_cast<int16_t>(std::round(ridgePtY2));

            totalVerices[lowerPolygonStartIdx + i] = engine::stl::array<int16_t, 4u>{
                finalX2, 
                static_cast<int16_t>(extentf - finalY2), 
                static_cast<int16_t>(hMax), 
                static_cast<int16_t>(1)
            };

            localNewRing.emplace_back(engine::vec2f{ridgePtX, ridgePtY});
        }

        earcutPolygonData.emplace_back(std::move(localNewRing));
    }

    // faces
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

    // top plane
    std::vector<uint32_t> newRidgeIndices = engine::earcut<uint32_t>(earcutPolygonData);
    for (auto idx : newRidgeIndices) {
        if (idx < vertices.size()) {
            totalIndices.emplace_back(upperPolygonStartIdx + idx);
        }
    }
}

}