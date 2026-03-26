#pragma once

#include "math.h"

namespace engine {
    inline vec3f projection(const vec3f & a, const vec3f & b) noexcept { // project a to b
        const vec3f axis = glm::normalize(b);
        return axis * glm::dot(axis, a);
    }

    inline void normalizePlane(vec4f & plane) { // plane normalization
        const float invSqrt = inv_sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        plane *= invSqrt;
    }

    inline vec4f planeForm(const vec3f & p1, const vec3f & p2, const vec3f & p3) { // general plane equation with 3 points
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

    inline vec4f planeForm(const vec3f & normal, const vec3f & point) { // general plane equation with normal and point
        return { normal.x, normal.y, normal.z, -glm::dot(normal, point) };
    }

     inline float lenFromPointToLine(const vec3f & begin, const vec3f & end, const vec3f & p) {
        const vec3f direction = glm::normalize(end - begin);
        const vec3f directionToVec = p - begin;
        return glm::length(glm::cross(direction, directionToVec));
    }

    inline float lenFromPointToPlane(const vec4f & plane, const vec3f & p) {
        return plane.x * p.x + plane.y * p.y + plane.z * p.z + plane.w;
    }

    inline float lenFromLineToLine(const vec3f & begin, const vec3f & end, const vec3f & begin2, const vec3f & end2) {
        // http://www.cleverstudents.ru/line_and_plane/distance_between_skew_lines.html

        const vec3f direction = glm::normalize(end - begin); // line0 direction
        const vec3f direction2 = normalize(end2 - begin2); // line1 direction

        const vec3f cross = glm::cross(direction, direction2); // будет нормальный вектор плоскости, проходящей через прямую b параллельно прямой a

        const float D = -cross.x * begin2.x - cross.y * begin2.y - cross.z * begin2.z; // это чтобы проходил через b
        const float nMult = 1.0f / glm::length(cross); // нормирующий множитель

        const float len = glm::abs(cross.x * begin.x + cross.y * begin.y + cross.z * begin.z + D) * nMult; // чтобы параллельно a и длину сразу посчитаем

        return len;
    }

    inline vec3f reflectPointWithPlane(const vec4f& plane, const vec3f& p, const bool planeIsNormalized) {
        const vec3f planeNormal(plane.x, plane.y, plane.z);
        const float len = lenFromPointToPlane(plane, p);

        const float normalizer = planeIsNormalized ? 2.0f : 2.0f / (plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        return p - planeNormal * (normalizer * len);
    }

    inline vec3f intersectionLinePlane(const vec3f& a, const vec3f& b, const vec4f& plane) {
        const vec3f d = glm::normalize(b - a);
        const float t = -(plane.x * a.x + plane.y * a.y + plane.z * a.z + plane.w) / (plane.x * d.x + plane.y * d.y + plane.z * d.z);
        return { a.x + d.x * t, a.y + d.y * t, a.z + d.z * t };
    }

    inline vec3f crossingLinesPoint(const vec3f& a, const vec3f& b, const vec3f& c, const vec3f& d, const float eps) {
        const vec3f v_v = c + glm::cross(d - c, b - a);
        const vec4f pl = planeForm(c, d, v_v);
        const vec3f crossing = intersectionLinePlane(a, b, pl);
        return lenFromPointToLine(c, d, crossing) <= eps ? crossing : vec3f(std::numeric_limits<float>::max());
    }

    inline float triangleSquare(const float a, const float b, const float c) { // Heron's formula
        const float p = (a + b + c) * 0.5f;
        const float sqrtv = 1.0f / inv_sqrt(p * (p - a) * (p - b) * (p - c));
        return sqrtv;
    }
}