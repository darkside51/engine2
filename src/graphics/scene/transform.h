#pragma once

#include "math/math.h"

namespace engine::scene {

class Transformation {
public:
    Transformation() = default;
    explicit Transformation(float scale) : _scale(vec3f{scale}) {}
    explicit Transformation(const vec3f & scale) : _scale(scale) {}

    quatf getRotation() const noexcept { return _rotation; }
    vec3f getRotationAngles() const noexcept { return glm::eulerAngles(_rotation); }
    vec3f getScale() const noexcept { return _scale; }
    vec3f getPosition() const noexcept { return _position; }

    bool setRotation(const quatf & q) noexcept {
        if (compare(_rotation, q, 1e-5f)) {
            _rotation = q;
            return true;
        }
        return false;
    }

    bool setRotation(const vec3f & angles) noexcept {
        const quatf q{angles};
        if (compare(_rotation, q, 1e-5f)) {
            _rotation = q;
            return true;
        }
        return false;
    }

    bool setScale(const vec3f & s) noexcept {
        if (compare(_scale, s, 1e-7f)) {
            _scale = s;
            return true;;
        }
        return false;
    }

    bool setScale(float scale) noexcept {
        const vec3f s{scale};
        if (compare(_scale, s, 1e-7f)) {
            _scale = s;
            return true;;
        }
        return false;
    }

    bool setPosition(const vec3f & p) noexcept {
        if (compare(_position, p, 1e-7f)) {
            _position = p;
            return true;
        }
        return false;
    }

    void apply(mat4f & matrix) noexcept {
        matrix = makeMatrix(_scale);
        matrix = glm::mat4_cast(_rotation) * matrix;
        matrix[3] = {_position, 1.0f};
    }

private:
    quatf _rotation{};
    vec3f _scale{1.0f};
    vec3f _position{0.0f};
};

}