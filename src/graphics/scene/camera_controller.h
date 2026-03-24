#pragma once

#include "camera.h"
#include <limits>
#include <cmath>

namespace engine::scene {

class CameraController {
public:
    CameraController(Camera & camera) : _camera(camera) { }

    const vec3f & getScale() const noexcept { return _scale; }
	const vec3f & getRotation() const noexcept { return _rotation; }
	const vec3f & getPosition() const noexcept { return _position; }

    void setRotationOrder(RotationsOrder ro) noexcept {
        if (_rotationOrder != ro) {
            _rotationOrder = ro;
            _dirty = true;
        }
    }

    void setScale(const vec3f & s) noexcept {
        if (compare(_scale, s, kEps)) {
            _scale = s;
            _dirty = true;
        }
    }

    void setRotation(const vec3f & r) noexcept {
        if (compare(_rotation, r, kEps)) {
            _rotation = r;
            _dirty = true;
        }
    }

    void setPosition(const vec3f & p) noexcept {
        if (compare(_position, p, kEps)) {
            _position = p;
            _dirty = true;
        }
    }

    void movePosition(const vec3f & p) noexcept {
        if (compare(p, emptyVec3, kEps)) {
            const vec4f move = vec4f(p, 0.0f) * _camera.getTransform();

            _position.x += move.x;
            _position.y += move.y;
            _position.z += move.z;

            _dirty = true;
        }
    }

    bool calculateTransform() noexcept {
        if (!_dirty && !_camera.transformDirty()) {
            return false;
        }

        _dirty = false;

		mat4f view{1.0f};
		
		if (_scale != unitVec3) {
			scaleMatrix(view, _scale);
		}

		if (_rotation != emptyVec3) {
			rotateMatrix_byOrder(view, _rotation, _rotationOrder);
		}

		if (_position != emptyVec3) {
			translateMatrix(view, -_position);
		}

        _camera.setView(view);
        
        return _camera.calculateTransform();
    }

protected:
    inline static constexpr float kEps = 1e-6f;
    RotationsOrder _rotationOrder = RotationsOrder::eXYZ;
    bool _dirty = false;
    vec3f _scale{1.0f};
    vec3f _rotation{0.0f};
    vec3f _position{0.0f};

    Camera & _camera;
};

class SphereCameraController : public CameraController {
public:
    SphereCameraController(Camera & camera) : CameraController(camera) { }

    float getDistance() const noexcept { return _distance; }

    void setDistance(float distance) noexcept { 
        if (std::abs(distance - _distance) > kEps) {
            _distance = std::min(std::max(distance, _minDistance), _maxDistance);
            _dirty = true;
        }
    }

    void setMinMaxDistance(float minDistance, float maxDistance) noexcept {
        _minDistance = minDistance;
        _maxDistance = maxDistance;
    }

    bool calculateTransform() noexcept {
        if (!_dirty && !_camera.transformDirty()) {
            return false;
        }

        _dirty = false;

		mat4f view{1.0f};
		
		if (_scale != unitVec3) {
			scaleMatrix(view, _scale);
		}

		if (_rotation != emptyVec3) {
			rotateMatrix_byOrder(view, _rotation, _rotationOrder);
		}

        const float rx = _rotation.x;
        const float rz = _rotation.z;
        _cameraPosition = _position;

        if (std::abs(_distance) > kEps) {
            // use spherical coordinates
            const float crx = std::cos(rx);
            const float crz = std::cos(rz);
            const float srx = std::sin(rx);
            const float srz = std::sin(rz);

            const float x = _distance * srx * srz;
            const float y = _distance * srx * crz;
            const float z = _distance * crx;

            _cameraPosition += vec3f{ x, y, z };
        }

        if (_cameraPosition != emptyVec3) {
			translateMatrix(view, -_cameraPosition);
		}

        _camera.setView(view);
        
        return _camera.calculateTransform();
    }

    const vec3f & cameraPosition() const noexcept { return _cameraPosition; }

protected:
    float _distance = 0.0f;
    float _minDistance = 0.0f;
    float _maxDistance = std::numeric_limits<float>::max(); 
    vec3f _cameraPosition = {};
};

}