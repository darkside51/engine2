#include "vertex_processing.h"

namespace engine::graphics {

stl::vector<vec4f> generateTangents(
    stl::span<const vec3f> positions,
    stl::span<const vec3f> normals,
    stl::span<const vec2f> uvs,
    stl::span<const uint32_t> indices) {
    const size_t vertexCount = positions.size();
    const size_t indexCount = indices.size();
    
    // temporary accumulation arrays
    stl::vector<vec3f> tan1(vertexCount, {0.0f, 0.0f, 0.0f});
    stl::vector<vec3f> tan2(vertexCount, {0.0f, 0.0f, 0.0f});

    // step 1: calculate face tangents and accumulate per vertex
    for (size_t i = 0u; i < indexCount; i += 3u) {
        const uint32_t i1 = indices[i];
        const uint32_t i2 = indices[i + 1u];
        const uint32_t i3 = indices[i + 2u];

        const vec3f & v1 = positions[i1];
        const vec3f & v2 = positions[i2];
        const vec3f & v3 = positions[i3];

        const vec2f & w1 = uvs[i1];
        const vec2f & w2 = uvs[i2];
        const vec2f & w3 = uvs[i3];

        const float x1 = v2.x - v1.x;
        const float x2 = v3.x - v1.x;
        const float y1 = v2.y - v1.y;
        const float y2 = v3.y - v1.y;
        const float z1 = v2.z - v1.z;
        const float z2 = v3.z - v1.z;

        const float s1 = w2.x - w1.x;
        const float s2 = w3.x - w1.x;
        const float t1 = w2.y - w1.y;
        const float t2 = w3.y - w1.y;

        // prevent division by zero if UVs are degenerate
        const float r = (s1 * t2 - s2 * t1);
        const float div = (r == 0.0f) ? 1.0f : (1.0f / r);

        const vec3f sdir = {
            (t2 * x1 - t1 * x2) * div,
            (t2 * y1 - t1 * y2) * div,
            (t2 * z1 - t1 * z2) * div
        };

        const vec3f tdir = {
            (s1 * x2 - s2 * x1) * div,
            (s1 * y2 - s2 * y1) * div,
            (s1 * z2 - s2 * z1) * div
        };

        tan1[i1] = tan1[i1] + sdir;
        tan1[i2] = tan1[i2] + sdir;
        tan1[i3] = tan1[i3] + sdir;

        tan2[i1] = tan2[i1] + tdir;
        tan2[i2] = tan2[i2] + tdir;
        tan2[i3] = tan2[i3] + tdir;
    }

    stl::vector<vec4f> tangents{vertexCount};

    // step 2: orthogonalize and calculate handedness (w)
    for (size_t i = 0u; i < vertexCount; ++i) {
        const vec3f & n = normals[i];
        const vec3f & t = tan1[i];

        // Gram-Schmidt orthogonalization
        // t_orthogonal = t - n * dot(n, t)
        const vec3f tangent = glm::normalize(t - (n * glm::dot(n, t)));

        // calculate handedness (bitangent flip factor)
        // sign is determined by whether the cross product matches tan2
        const vec3f cross_n_t = glm::cross(n, tangent);
        const float handedness = (glm::dot(cross_n_t, tan2[i]) < 0.0f) ? -1.0f : 1.0f;

        tangents[i] = {tangent.x, tangent.y, tangent.z, handedness};
    }

    return tangents;
}

stl::vector<std::byte> generateTangentsCompressed(
    stl::span<const uint16_t> positions,
    stl::span<const int8_t> normals,
    stl::span<const uint16_t> uvs,
    stl::span<const uint32_t> indices
) {
    const size_t vertexCount = positions.size() / 4u;
    const size_t indexCount = indices.size();

    // temporary accumulation arrays
    stl::vector<vec3f> tan1(vertexCount, {0.0f, 0.0f, 0.0f});
    stl::vector<vec3f> tan2(vertexCount, {0.0f, 0.0f, 0.0f});

    // step 1: calculate face tangents and accumulate per vertex
    for (size_t i = 0u; i < indexCount; i += 3u) {
        const uint32_t i1 = indices[i];
        const uint32_t i2 = indices[i + 1u];
        const uint32_t i3 = indices[i + 2u];

        const vec3f v1 = {
            uint16_to_float(positions[i1 * 4u + 0u]),
            uint16_to_float(positions[i1 * 4u + 1u]),
            uint16_to_float(positions[i1 * 4u + 2u])
        };

        const vec3f v2 = {
            uint16_to_float(positions[i2 * 4u + 0u]),
            uint16_to_float(positions[i2 * 4u + 1u]),
            uint16_to_float(positions[i2 * 4u + 2u])
        };

        const vec3f v3 = {
            uint16_to_float(positions[i3 * 4u + 0u]),
            uint16_to_float(positions[i3 * 4u + 1u]),
            uint16_to_float(positions[i3 * 4u + 2u])
        };

        const vec2f w1 = {
            uint16_to_float(uvs[i1 * 2u + 0u]),
            uint16_to_float(uvs[i1 * 2u + 1u])
        };

        const vec2f w2 = {
            uint16_to_float(uvs[i2 * 2u + 0u]),
            uint16_to_float(uvs[i2 * 2u + 1u])
        };

        const vec2f w3 = {
            uint16_to_float(uvs[i3 * 2u + 0u]),
            uint16_to_float(uvs[i3 * 2u + 1u])
        };
        
        const float x1 = v2.x - v1.x;
        const float x2 = v3.x - v1.x;
        const float y1 = v2.y - v1.y;
        const float y2 = v3.y - v1.y;
        const float z1 = v2.z - v1.z;
        const float z2 = v3.z - v1.z;

        const float s1 = w2.x - w1.x;
        const float s2 = w3.x - w1.x;
        const float t1 = w2.y - w1.y;
        const float t2 = w3.y - w1.y;

        // prevent division by zero if UVs are degenerate
        const float r = (s1 * t2 - s2 * t1);
        const float div = (r == 0.0f) ? 1.0f : (1.0f / r);

        const vec3f sdir = {
            (t2 * x1 - t1 * x2) * div,
            (t2 * y1 - t1 * y2) * div,
            (t2 * z1 - t1 * z2) * div
        };

        const vec3f tdir = {
            (s1 * x2 - s2 * x1) * div,
            (s1 * y2 - s2 * y1) * div,
            (s1 * z2 - s2 * z1) * div
        };

        tan1[i1] = tan1[i1] + sdir;
        tan1[i2] = tan1[i2] + sdir;
        tan1[i3] = tan1[i3] + sdir;

        tan2[i1] = tan2[i1] + tdir;
        tan2[i2] = tan2[i2] + tdir;
        tan2[i3] = tan2[i3] + tdir;
    }

    stl::vector<std::byte> tangents{vertexCount * 4u};

    // step 2: orthogonalize and calculate handedness (w)
    for (size_t i = 0u; i < vertexCount; ++i) {
        const vec3f n = {
            snorm_to_float(normals[i * 4u + 0u], 8u), 
            snorm_to_float(normals[i * 4u + 1u], 8u), 
            snorm_to_float(normals[i * 4u + 2u], 8u)
        };

        const vec3f & t = tan1[i];

        // Gram-Schmidt orthogonalization
        // t_orthogonal = t - n * dot(n, t)
        const vec3f tangent = glm::normalize(t - (n * glm::dot(n, t)));

        // calculate handedness (bitangent flip factor)
        // sign is determined by whether the cross product matches tan2
        const vec3f cross_n_t = glm::cross(n, tangent);
        const float handedness = (glm::dot(cross_n_t, tan2[i]) < 0.0f) ? -1.0f : 1.0f;

        tangents[i * 4u + 0u] = static_cast<std::byte>(float_to_snorm(tangent.x, 8u));
        tangents[i * 4u + 1u] = static_cast<std::byte>(float_to_snorm(tangent.y, 8u));
        tangents[i * 4u + 2u] = static_cast<std::byte>(float_to_snorm(tangent.z, 8u));
        tangents[i * 4u + 3u] = static_cast<std::byte>(float_to_snorm(handedness, 8u));
    }

    return tangents;
}

}