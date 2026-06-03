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



// void generateTangents(
//     const std::vector<Vertex>& vertices,
//     const std::vector<unsigned int>& indices,
//     std::vector<Vector4>& outTangents) 
// {
//     size_t vertexCount = vertices.size();
//     size_t indexCount = indices.size();
    
//     outTangents.resize(vertexCount, {0.0f, 0.0f, 0.0f, 1.0f});
    
//     // Temporary accumulation arrays
//     std::vector<Vector3> tan1(vertexCount, {0.0f, 0.0f, 0.0f});
//     std::vector<Vector3> tan2(vertexCount, {0.0f, 0.0f, 0.0f});

//     // Step 1: Calculate face tangents and accumulate per vertex
//     for (size_t i = 0; i < indexCount; i += 3) {
//         unsigned int i1 = indices[i];
//         unsigned int i2 = indices[i + 1];
//         unsigned int i3 = indices[i + 2];

//         const Vector3& v1 = vertices[i1].position;
//         const Vector3& v2 = vertices[i2].position;
//         const Vector3& v3 = vertices[i3].position;

//         const Vector2& w1 = vertices[i1].uv;
//         const Vector2& w2 = vertices[i2].uv;
//         const Vector2& w3 = vertices[i3].uv;

//         float x1 = v2.x - v1.x;
//         float x2 = v3.x - v1.x;
//         float y1 = v2.y - v1.y;
//         float y2 = v3.y - v1.y;
//         float z1 = v2.z - v1.z;
//         float z2 = v3.z - v1.z;

//         float s1 = w2.x - w1.x;
//         float s2 = w3.x - w1.x;
//         float t1 = w2.y - w1.y;
//         float t2 = w3.y - w1.y;

//         // Prevent division by zero if UVs are degenerate
//         float r = (s1 * t2 - s2 * t1);
//         float div = (r == 0.0f) ? 1.0f : (1.0f / r);

//         Vector3 sdir = {
//             (t2 * x1 - t1 * x2) * div,
//             (t2 * y1 - t1 * y2) * div,
//             (t2 * z1 - t1 * z2) * div
//         };

//         Vector3 tdir = {
//             (s1 * x2 - s2 * x1) * div,
//             (s1 * y2 - s2 * y1) * div,
//             (s1 * z2 - s2 * z1) * div
//         };

//         tan1[i1] = tan1[i1] + sdir;
//         tan1[i2] = tan1[i2] + sdir;
//         tan1[i3] = tan1[i3] + sdir;

//         tan2[i1] = tan2[i1] + tdir;
//         tan2[i2] = tan2[i2] + tdir;
//         tan2[i3] = tan2[i3] + tdir;
//     }

//     // Step 2: Orthogonalize and calculate handedness (w)
//     for (size_t a = 0; a < vertexCount; ++a) {
//         const Vector3& n = vertices[a].normal;
//         const Vector3& t = tan1[a];

//         // Gram-Schmidt orthogonalization
//         // t_orthogonal = t - n * dot(n, t)
//         Vector3 tangentxyz = t - (n * Vector3::dot(n, t));
//         tangentxyz.normalize();

//         // Calculate handedness (bitangent flip factor)
//         // Sign is determined by whether the cross product matches tan2
//         Vector3 cross_n_t = Vector3::cross(n, tangentxyz);
//         float handedness = (Vector3::dot(cross_n_t, tan2[a]) < 0.0f) ? -1.0f : 1.0f;

//         outTangents[a] = {tangentxyz.x, tangentxyz.y, tangentxyz.z, handedness};
//     }
// }

}