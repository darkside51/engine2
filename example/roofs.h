#pragma once

#include "math/math.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

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
);

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
);

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
);

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
);

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
);

}