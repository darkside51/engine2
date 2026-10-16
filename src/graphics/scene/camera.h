#pragma once

#include "math/math.h"
#include "stl/vector.h"

#include <cstdint>

namespace engine::scene {

class Camera;

class ICameraObserver {
public:
    virtual ~ICameraObserver() = default;
    virtual void onCameraChanged(const Camera & camera) = 0;
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
    Interval<3u, float> screenToWorld(vec2f screenCoord) noexcept;

    bool calculateTransform() noexcept;

    void makePerspective(float fov, float aspect, float znear, float zfar) noexcept;
    void makeOrtho(float left, float right, float bottom, float top, float znear, float zfar) noexcept;

    void setView(const mat4f & view) noexcept;
    float getAspect() const noexcept;

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

    stl::vector<ICameraObserver*> _observers;
};

}