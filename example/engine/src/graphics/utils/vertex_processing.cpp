#include "vertex_processing.h"

namespace engine::graphics {

stl::vector<vec4f> generateTangents(
    stl::span<const vec3f> positions,
    stl::span<const vec3f> normals,
    stl::span<const vec2f> uvs,
    stl::span<const uint32_t> indices) {
    constexpr float kEps = 1e-9f;
    const size_t vertexCount = positions.size();
    const size_t indexCount = indices.size();
    
    // temporary accumulation arrays
    stl::vector<vec3f> tan1(vertexCount, {0.0f, 0.0f, 0.0f});
    stl::vector<vec3f> tan2(vertexCount, {0.0f, 0.0f, 0.0f});

    // calculate face tangents and accumulate per vertex
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

    // orthogonalize and calculate handedness (w)
    for (size_t i = 0u; i < vertexCount; ++i) {
        vec3f n = normals[i];
        const vec3f & t = tan1[i];

        // normalize the normal (fallback keeps the basis valid for degenerate normals)
        if (math::length2(n) > kEps) {
            n = math::normalize(n);
        } else {
            n = vec3f{0.0f, 0.0f, 1.0f};
        }

        // Gram-Schmidt orthogonalization
        // t_orthogonal = t - n * dot(n, t)
        vec3f tangent = t - (n * math::dot(n, t));

        if (math::length2(tangent) > kEps) {
            tangent = math::normalize(tangent);
        } else {
            // degenerate case: the accumulated tangent is parallel to the normal
            // (or the UVs are degenerate) - build an arbitrary perpendicular vector
            // so the TBN basis stays valid instead of producing NaNs
            const vec3f axis = (n.x * n.x < 0.81f) ? vec3f{1.0f, 0.0f, 0.0f} : vec3f{0.0f, 1.0f, 0.0f};
            tangent = math::normalize(math::cross(n, axis));
        }

        // calculate handedness (bitangent flip factor)
        // sign is determined by whether the cross product matches tan2
        const vec3f cross_n_t = math::cross(n, tangent);
        const float handedness = (math::dot(cross_n_t, tan2[i]) < 0.0f) ? -1.0f : 1.0f;

        tangents[i] = {tangent.x, tangent.y, tangent.z, handedness};
    }

    return tangents;
}

stl::vector<std::byte> generateTangentsCompressed(
    stl::span<const vec3f> positions,
    stl::span<const vec3f> normals,
    stl::span<const vec2f> uvs,
    stl::span<const uint32_t> indices
) {
    // full precision tangents are generated first and only the result is quantized
    const stl::vector<vec4f> tangents = generateTangents(positions, normals, uvs, indices);

    stl::vector<std::byte> compressed(tangents.size() * 4u);

    for (size_t i = 0u; i < tangents.size(); ++i) {
        const vec4f & t = tangents[i];

        compressed[i * 4u + 0u] = static_cast<std::byte>(float_to_snorm(t.x, 8u));
        compressed[i * 4u + 1u] = static_cast<std::byte>(float_to_snorm(t.y, 8u));
        compressed[i * 4u + 2u] = static_cast<std::byte>(float_to_snorm(t.z, 8u));
        compressed[i * 4u + 3u] = static_cast<std::byte>(float_to_snorm(t.w, 8u));
    }

    return compressed;
}

}