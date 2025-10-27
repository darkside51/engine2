#pragma once

#include "math/math.h"

#include <array>
#include <cstdint>

namespace engine::scene {

struct Bounds;

class Frustum {
public:
    Frustum() = default;
    Frustum(const mat4f & clip);
    
    void calculate(const mat4f & clip) noexcept;
    
    bool checkPoint(const vec3f & p) const noexcept;
    bool checkSphere(const vec3f & p, float r) const noexcept;
    bool checkCube(const vec3f & min, const vec3f & max) const noexcept;
    bool checkBounds(const Bounds & bounds) const noexcept;

private:
    std::array<vec4f, 6u> _frustum;
};

}