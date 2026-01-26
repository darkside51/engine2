#pragma once

#include "math/math.h"
#include "meshoptimizer.h"

#include <array>
#include <cstdint>

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

// vec2
inline std::array<uint16_t, 2u> to_vec2u16(vec2f v) noexcept {
    return {float_to_uint16(v.x), float_to_uint16(v.y)};
}

inline std::array<uint16_t, 2u> to_vec2u16(std::array<float, 2u> v) noexcept {
    return {float_to_uint16(v[0u]), float_to_uint16(v[1u])};
}

inline std::array<uint16_t, 2u> to_vec2u16(float x, float y) noexcept {
    return {float_to_uint16(x), float_to_uint16(y)};
}

// vec3
inline std::array<uint16_t, 3u> to_vec3u16(vec3f v) noexcept {
    return {float_to_uint16(v.x), float_to_uint16(v.y), float_to_uint16(v.z)};
}

inline std::array<uint16_t, 3u> to_vec3u16(std::array<float, 3u> v) noexcept {
    return {float_to_uint16(v[0u]), float_to_uint16(v[1u]), float_to_uint16(v[2u])};
}

inline std::array<uint16_t, 3u> to_vec3u16(float x, float y, float z) noexcept {
    return {float_to_uint16(x), float_to_uint16(y), float_to_uint16(z)};
}

}