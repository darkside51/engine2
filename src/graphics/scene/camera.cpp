#include "camera.h"

namespace engine::scene {

    void Camera::makePerspective(float fov, float aspect, float znear, float zfar) noexcept {
        _type = ProjectionType::ePerspective;
		_projection = glm::perspectiveRH_ZO(fov, aspect, znear, zfar);
		_projection[1][1] *= -1.0f;
		_dirty->transform = 1u;
    }

    void Camera::makeOrtho(float left, float right, float bottom, float top, float znear, float zfar) noexcept {
        _type = ProjectionType::eOrtho;
		_projection = glm::orthoRH_ZO(left, right, bottom, top, znear, zfar);
		_projection[3][2] = 0.0f;
		_projection[1][1] *= -1.0f;
		_dirty->transform = 1u;
    }

    void Camera::resize(float w, float h) noexcept {
        if (_size.x == w && _size.y == h) {
            return;
        }

		switch (_type) {
			case ProjectionType::ePerspective:
			{
				_projection[0][0] = -_projection[1][1] * (h / w);
			}
				break;
			case ProjectionType::eOrtho:
			{
				_projection[0][0] *= (_size.x / w);
				_projection[1][1] *= (_size.y / h);
			}
				break;
			default:
				break;
		}

		_size.x = w;
		_size.y = h;
		_dirty->transform = 1u;
	}

	float Camera::getAspect() const noexcept {
		return _transform[0u][0u] / _transform[1u][1u];
	}

    vec2f Camera::worldToScreen(const vec3f & p) const noexcept {
		return (worldToNdc(_transform, p) * 0.5f + 0.5f) * _size;
	}

	Interval<3u, float> Camera::screenToWorld(vec2f screenCoord) noexcept {
		return ndcToWorld(getInvTransform(), ((2.0f * screenCoord) / _size) - 1.0f);
	}

    void Camera::setView(const mat4f & view) noexcept {
        if (memcmp(&_view, &view, sizeof(mat4f))) {
            _view = view;
            _dirty->transform = 1u;
        }
    }

    bool Camera::calculateTransform() noexcept {
        if (_dirty->transform == 0u) {
            return false;
        }

        _transform = _projection * _view;

		_dirty->transform = 0u;
		_dirty->inverseTransform = 1u;
		_dirty->inverseView = 1u;

        notifyObservers();
        
        return true;
    }
}