#include "camera.h"

namespace engine::scene {

    void Camera::makePerspective(float fov, float aspect, float znear, float zfar) noexcept {
        _type = ProjectionType::ePerspective;
		_projection = glm::perspective(fov, aspect, znear, zfar);
		_projection[1][1] *= -1.0f;
		_dirty->transform = 1u;
    }

    void Camera::makeOrtho(float left, float right, float bottom, float top, float znear, float zfar) noexcept {
        _type = ProjectionType::eOrtho;
		_projection = glm::ortho(left, right, bottom, top, znear, zfar);
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

    vec2f Camera::worldToScreen(const vec3f & p) const noexcept {
		vec4f v = _transform * vec4f(p, 1.0f);
		v /= v.w;

		v.x = (0.5f + v.x * 0.5f) * _size.x;
		v.y = (0.5f - v.y * 0.5f) * _size.y; // for topdown viewport oy

		return vec2f(v.x, v.y);
	}

	Ray Camera::screenToWorld(const vec2f & screenCoord) noexcept {
		vec4f v;
		v.x = (((2.0f * screenCoord.x) / _size.x) - 1.0f);
		v.y = -(((2.0f * screenCoord.y) / _size.y) - 1.0f); // for topdown viewport oy
		v.z = 0.0f;
		v.w = 1.0f;

		vec4f sp = getInvTransform() * v;
		sp /= sp.w;

		v.z = 1.0f;

		vec4f fp = _inverseTransform * v;
		fp /= fp.w;

		return Ray(sp.x, sp.y, sp.z, fp.x, fp.y, fp.z);
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