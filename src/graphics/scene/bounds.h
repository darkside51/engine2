#pragma once

#include "math/math.h"
#include <cstdint>

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
    };

}