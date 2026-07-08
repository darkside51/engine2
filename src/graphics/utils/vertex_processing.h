#pragma once

#include "math/math.h"
#include "stl/array.h"
#include "stl/span.h"
#include "stl/vector.h"
#include "meshoptimizer.h"

#include <cstddef>
#include <cstdint>

#include <span>
#include <vector>

namespace engine::graphics {

inline uint16_t float_to_uint16(float v) noexcept {
    return meshopt_quantizeHalf(v);
}

inline float uint16_to_float(uint16_t v) noexcept {
    return meshopt_dequantizeHalf(v);
}

inline int32_t float_to_snorm(float v, int n) noexcept {
    return meshopt_quantizeSnorm(v, n);
}

inline int32_t float_to_unorm(float v, int n) noexcept {
    return meshopt_quantizeUnorm(v, n);
}

inline float snorm_to_float(int32_t quantized, uint8_t bits) noexcept {
    const int32_t max_val = (1 << (bits - 1)) - 1;
    const float value = static_cast<float>(quantized) / max_val;
    return std::clamp(value, -1.0f, 1.0f);
};

inline float unorm_to_float(int32_t quantized, uint8_t bits) noexcept {
    const auto max_val = static_cast<float>((1 << (bits)) - 1);
    return static_cast<float>(quantized) / max_val;
};

// vec2
inline stl::array<uint16_t, 2u> to_vec2u16(vec2f v) noexcept {
    return {float_to_uint16(v.x), float_to_uint16(v.y)};
}

inline stl::array<uint16_t, 2u> to_vec2u16(std::array<float, 2u> v) noexcept {
    return {float_to_uint16(v[0u]), float_to_uint16(v[1u])};
}

inline stl::array<uint16_t, 2u> to_vec2u16(float x, float y) noexcept {
    return {float_to_uint16(x), float_to_uint16(y)};
}

// vec3
inline stl::array<uint16_t, 3u> to_vec3u16(vec3f v) noexcept {
    return {float_to_uint16(v.x), float_to_uint16(v.y), float_to_uint16(v.z)};
}

inline stl::array<uint16_t, 3u> to_vec3u16(std::array<float, 3u> v) noexcept {
    return {float_to_uint16(v[0u]), float_to_uint16(v[1u]), float_to_uint16(v[2u])};
}

inline stl::array<uint16_t, 3u> to_vec3u16(float x, float y, float z) noexcept {
    return {float_to_uint16(x), float_to_uint16(y), float_to_uint16(z)};
}

// generates 4D tangents Lengyel's algorithm
std::vector<vec4f> generateTangents(
    std::span<const vec3f> positions,
    std::span<const vec3f> normals,
    std::span<const vec2f> uvs,
    std::span<const uint32_t> indices
);

std::vector<std::byte> generateTangentsCompressed(
    std::span<const uint16_t> positions,
    std::span<const int8_t> normals,
    std::span<const uint16_t> uvs,
    std::span<const uint32_t> indices
);

}