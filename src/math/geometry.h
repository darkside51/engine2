#pragma once

#include "math.h"
#include "earcut.hpp"

#include <span>

namespace engine {
    bool intersectRays2D(const ray2f & r1, const ray2f & r2, vec2f & outPoint) noexcept {
        const auto cross = r1.direction.x * r2.direction.y - r1.direction.y * r2.direction.x;
        if (abs(cross) < 1e-7f) {
            return false; // the rays are parallel
        }

        const vec2f d = r2.origin - r1.origin;
        const auto t1 = (d.x * r2.direction.y - d.y * r2.direction.x) / cross;
        const auto t2 = (d.x * r1.direction.y - d.y * r1.direction.x) / cross;

        if (t1 >= 0.0f && t2 >= 0.0f) {
            outPoint = {r1.origin.x + t1 * r1.direction.x, r1.origin.y + t1 * r1.direction.y};
            return true;
        }
        return false; // the intersection is behind one of the rays
    }

    inline vec3f projection(const vec3f & a, const vec3f & b) noexcept { // project a to b
        const vec3f axis = math::normalize(b);
        return axis * math::dot(axis, a);
    }

    inline void normalizePlane(vec4f & plane) noexcept { // plane normalization
        const float invSqrt = inv_sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        plane *= invSqrt;
    }

    inline vec4f planeForm(const vec3f & p1, const vec3f & p2, const vec3f & p3) noexcept { // general plane equation with 3 points
        const float a = p2.x - p1.x;
        const float b = p2.y - p1.y;
        const float c = p2.z - p1.z;

        const float d = p3.x - p1.x;
        const float e = p3.y - p1.y;
        const float f = p3.z - p1.z;

        const float pA = b * f - e * c;
        const float pB = d * c - a * f;
        const float pC = a * e - d * b;

        const float pD = -p1.x * pA - p1.y * pB - p1.z * pC;
        return { pA, pB, pC, pD };
    }

    inline vec4f planeForm(const vec3f & normal, const vec3f & point) noexcept { // general plane equation with normal and point
        return { normal.x, normal.y, normal.z, -math::dot(normal, point) };
    }

    inline float lenFromPointToLine(const vec3f & begin, const vec3f & end, const vec3f & p) noexcept {
        const vec3f direction = math::normalize(end - begin);
        const vec3f directionToVec = p - begin;
        return math::length(math::cross(direction, directionToVec));
    }

    inline float len2FromPointToLine(const vec3f & begin, const vec3f & end, const vec3f & p) noexcept {
        const vec3f direction = math::normalize(end - begin);
        const vec3f directionToVec = p - begin;
        return math::length2(math::cross(direction, directionToVec));
    }

    inline float lenFromPointToPlane(const vec4f & plane, const vec3f & p) noexcept {
        return plane.x * p.x + plane.y * p.y + plane.z * p.z + plane.w;
    }

    inline float lenFromLineToLine(const vec3f & begin, const vec3f & end, const vec3f & begin2, const vec3f & end2) noexcept {
        // http://www.cleverstudents.ru/line_and_plane/distance_between_skew_lines.html

        const vec3f direction = math::normalize(end - begin); // line0 direction
        const vec3f direction2 = normalize(end2 - begin2); // line1 direction

        const vec3f cross = math::cross(direction, direction2); // будет нормальный вектор плоскости, проходящей через прямую b параллельно прямой a

        const float D = -cross.x * begin2.x - cross.y * begin2.y - cross.z * begin2.z; // это чтобы проходил через b
        const float nMult = 1.0f / math::length(cross); // нормирующий множитель

        const float len = math::abs(cross.x * begin.x + cross.y * begin.y + cross.z * begin.z + D) * nMult; // чтобы параллельно a и длину сразу посчитаем

        return len;
    }

    inline vec3f reflectPointWithPlane(const vec4f & plane, const vec3f & p, const bool planeIsNormalized) noexcept {
        const vec3f planeNormal(plane.x, plane.y, plane.z);
        const float len = lenFromPointToPlane(plane, p);

        const float normalizer = planeIsNormalized ? 2.0f : 2.0f / (plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        return p - planeNormal * (normalizer * len);
    }

    inline vec3f intersectionLinePlane(const vec3f & a, const vec3f & b, const vec4f & plane) noexcept {
        const vec3f d = math::normalize(b - a);
        const float t = -(plane.x * a.x + plane.y * a.y + plane.z * a.z + plane.w) / (plane.x * d.x + plane.y * d.y + plane.z * d.z);
        return { a.x + d.x * t, a.y + d.y * t, a.z + d.z * t };
    }

    inline vec3f crossingLinesPoint(const vec3f & a, const vec3f & b, const vec3f & c, const vec3f & d, const float eps) noexcept {
        const vec3f v_v = c + math::cross(d - c, b - a);
        const vec4f pl = planeForm(c, d, v_v);
        const vec3f crossing = intersectionLinePlane(a, b, pl);
        return lenFromPointToLine(c, d, crossing) <= eps ? crossing : vec3f(std::numeric_limits<float>::max());
    }

    inline float triangleSquare(float a, float b, float c) noexcept { // Heron's formula
        const float p = (a + b + c) * 0.5f;
        const float sqrtv = 1.0f / inv_sqrt(p * (p - a) * (p - b) * (p - c));
        return sqrtv;
    }

    template <typename VEC>
    bool isPointInPolygon2D(const VEC & p, stl::span<const VEC> points) noexcept {
        // https://gamedev.ru/code/forum/?id=42313
        const size_t psz = points.size();
        if (psz < 3u) {
            return false;
        }

        bool result = false;
        VEC last = points.back();
        for (auto const & pt : points) {
            if ((pt.y > p.y) ^ (last.y > p.y)) {
                auto const t = static_cast<VEC::value_type>(p.y - last.y) / (pt.y - last.y);
                auto const x = static_cast<VEC::value_type>(last.x) + t * (pt.x - last.x);
                if (x > p.x) {
                    result = !result;
                }
            }
            last = pt;
        }
        return result;
    }

    template<typename T>
    void calculateNormals(stl::span<T> vertices, stl::span<const uint32_t> indices) noexcept {
        for (auto & v : vertices) {
            v.normal = vec3f(0.0F);
        }

        for (auto i = 0u; i < indices.size(); i += 3u) {
            auto const idx0 = indices[i + 0u];
            auto const idx1 = indices[i + 1u];
            auto const idx2 = indices[i + 2u];

            auto const v02 = math::normalize(vertices[idx0].position - vertices[idx2].position);
            auto const v10 = math::normalize(vertices[idx1].position - vertices[idx0].position);
            auto const v21 = math::normalize(vertices[idx2].position - vertices[idx1].position);

            vertices[idx0].normal += math::normalize(math::cross(v02, v10));
            vertices[idx1].normal += math::normalize(math::cross(v10, v21));
            vertices[idx2].normal += math::normalize(math::cross(v21, v02));
        }

        for (auto & v : vertices) {
            v.normal = math::normalize(v.normal);
        }
    }
}