#pragma once

#include "math/math.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace engine::scene {

    struct Cube {
        vec3f corner0{0.0f};
        vec3f corner1{0.0f};
    };

    struct Sphere {
        vec3f center{0.0f};
        float radius = 0.0f;
    };

    union Volume {
        Cube cube{};
        Sphere sphere;
    };

    struct Bounds {
        enum class Type : uint8_t {
                eNone = 0u,
                eCube = 1u,
                eSphere = 2u
        } type = Type::eNone;
        Volume volume;

        static constexpr Bounds createCube(const vec3f & corner0, const vec3f & corner1) noexcept {
            return Bounds {
                .type = Type::eCube,
                .volume = Cube{.corner0 = corner0, .corner1 = corner1}
            };
        }

        static constexpr Bounds createSphere(const vec3f & center, float radius) noexcept {
            return Bounds {
                .type = Type::eSphere,
                .volume = Cube{.corner0 = center, .corner1 = {radius, 0.0f, 0.0f}}
            };
        }

        Bounds transform(const mat4f & transform) const noexcept {
            switch (type) {
                case Type::eSphere:
                {
                    const auto center = transform * vec4f{volume.sphere.center, 1.0f};
                    const float radius = glm::length(transform[0u]) * volume.sphere.radius;
                    return createSphere(center / center.w, radius);
                }
                case Type::eCube:
                {
                    vec4f m[8u];

                    auto const & min = volume.cube.corner0;
                    auto const & max = volume.cube.corner1;

                    m[0u] = transform * vec4f(min.x, min.y, min.z, 1.0f);
                    m[1u] = transform * vec4f(min.x, min.y, max.z, 1.0f);
                    m[2u] = transform * vec4f(min.x, max.y, min.z, 1.0f);
                    m[3u] = transform * vec4f(max.x, min.y, min.z, 1.0f);

                    m[4u] = transform * vec4f(min.x, max.y, max.z, 1.0f);
                    m[5u] = transform * vec4f(max.x, min.y, max.z, 1.0f);
                    m[6u] = transform * vec4f(max.x, max.y, min.z, 1.0f);
                    m[7u] = transform * vec4f(max.x, max.y, max.z, 1.0f);

                    vec3f m1{std::numeric_limits<float>::max()};
                    vec3f m2{std::numeric_limits<float>::lowest()};
                    for (uint8_t i = 0u; i < 8u; ++i) {
                        m[i] /= m[i].w;
                        m1.x = std::min(m1.x, m[i].x); m1.y = std::min(m1.y, m[i].y); m1.z = std::min(m1.z, m[i].z);
                        m2.x = std::max(m2.x, m[i].x); m2.y = std::max(m2.y, m[i].y); m2.z = std::max(m2.z, m[i].z);
                    }
                    return createCube(m1, m2);
                }
                default: return {};
            }
        }
    };

}