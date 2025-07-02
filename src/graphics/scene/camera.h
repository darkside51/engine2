#pragma once

#include "math/math.h"

#include <cstdint>
#include <vector>

namespace engine::scene {

class Camera;

class ICameraObserver {
public:
    virtual ~ICameraObserver() = default;
    virtual void onCameraChanged(const Camera & camera) = 0;
};

struct Ray {
    vec3f begin;
    vec3f end;
    Ray(
        float sx, float sy, float sz,
        float fx, float fy, float fz
    ) : begin(sx, sy, sz), end(fx, fy, fz) {}
};

enum class ProjectionType : uint8_t {
    eOrtho = 0u,
    ePerspective = 1u,
    eCustom = 2u
};

class Camera {
    struct DirtyFlags {
        uint8_t transform = 0u;
        uint8_t inverseTransform = 0u;
        uint8_t inverseView = 0u;
    };

    union DirtyValue {
        DirtyFlags flags;
        uint32_t v = 0u;

        DirtyValue(uint32_t value) : v(value) {}

        DirtyFlags* operator->() noexcept { return &flags; }
        const DirtyFlags* operator->() const noexcept { return &flags; }
    };

public:
    void resize(float width, float height) noexcept;

    const vec2f & getSize() const noexcept { return _size; }
    const mat4f & getTransform() const noexcept { return _transform; }
    const mat4f & getView() const noexcept { return _view; }
    const mat4f & getProjection() const noexcept { return _projection; }

    const mat4f & getInvTransform() noexcept {
        if (_dirty->inverseTransform) {
            _inverseTransform = glm::inverse(_transform);
            _dirty->inverseTransform = 0u;
        }
        return _inverseTransform;
    }

    const mat4f & getInvViewTransform() noexcept {
        if (_dirty->inverseView) {
            _inverseView = glm::inverse(_view);
            _dirty->inverseView = 0u;
        }
        return _inverseView;
    }

    bool transformDirty() const noexcept { return _dirty->transform != 0u; }
    vec2f worldToScreen(const vec3f & p) const noexcept;
    Ray screenToWorld(const vec2f & screenCoord) noexcept;

    bool calculateTransform() noexcept;

    void makePerspective(float fov, float aspect, float znear, float zfar) noexcept;
    void makeOrtho(float left, float right, float bottom, float top, float znear, float zfar) noexcept;

    void setView(const mat4f & view) noexcept;

    // observers
    inline void addObserver(ICameraObserver* observer) {
        _observers.push_back(observer);
    }

    inline void removeObserver(ICameraObserver* observer) {
        _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
    }

    inline void notifyObservers() noexcept {
        for (auto & observer : _observers) {
            observer->onCameraChanged(*this);
        }
    }
private:
    ProjectionType _type = ProjectionType::eCustom;
    DirtyValue _dirty{0u};

    vec2f _size{0u};
    mat4f _transform{1.0f};
    mat4f _inverseTransform{1.0f};

    mat4f _view{1.0f};
    mat4f _inverseView{1.0f};

    mat4f _projection{1.0f};

    std::vector<ICameraObserver*> _observers;
};

class CameraController {
    inline static constexpr float kEps = 1e-4f;
public:
    CameraController(Camera & camera) : _camera(camera) {
        
    }

    const vec3f & getScale() const noexcept { return _scale; }
	const vec3f & getRotation() const noexcept { return _rotation; }
	const vec3f & getPosition() const noexcept { return _position; }

    void setRotationOrder(const RotationsOrder ro) noexcept {
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
    RotationsOrder _rotationOrder = RotationsOrder::eXYZ;
    bool _dirty = false;
    vec3f _scale{1.0f};
    vec3f _rotation{0.0f};
    vec3f _position{0.0f};

    Camera & _camera;
};

class SphereCameraController final : public CameraController {
public:
    SphereCameraController(Camera & camera) : CameraController(camera) {
        
    }

    float getLen() const noexcept { return _len; }

    void setLen(float l) noexcept { 
        if (std::abs(l - _len) > 1e-4f) {
            _len = std::min(std::max(l, _minLen), _maxLen);
            _dirty = true;
        }
    }

    void setMinMaxLen(float minLen, float maxLen) noexcept {
        _minLen = minLen;
        _maxLen = maxLen;
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
        vec3f position = _position;

        if (std::abs(_len) > 1e-4f) {
            // use spherical coordinates
            const float crx = cosf(rx);
            const float crz = cosf(rz);
            const float srx = sinf(rx);
            const float srz = sinf(rz);

            const float x = _len * srx * srz;
            const float y = _len * srx * crz;
            const float z = _len * crx;

            position += vec3f{ x, y, z };
        }

        if (position != emptyVec3) {
			translateMatrix(view, -position);
		}

        _camera.setView(view);
        
        return _camera.calculateTransform();
    }

private:
    float _len = 0.0f;
    float _minLen = 0.0f;
    float _maxLen = 100.0f; 
};

}