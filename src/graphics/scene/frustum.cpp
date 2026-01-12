#include "frustum.h"
#include "bounds.h"

namespace engine::scene {

    Frustum::Frustum(const mat4f & clip) {
        calculate(clip);
    }

    void Frustum::calculate(const mat4f & clip) noexcept {
        // Gribb & Hartmann method for Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix:

        // get A,B,C,D for right plane
        _frustum[0u] = {clip[0u][3u] - clip[0u][0u], clip[1u][3u] - clip[1u][0u], clip[2u][3u] - clip[2u][0u],
                        clip[3u][3u] - clip[3u][0u]};
        // get A,B,C,D for left plane
        _frustum[1u] = {clip[0u][3u] + clip[0u][0u], clip[1u][3u] + clip[1u][0u], clip[2u][3u] + clip[2u][0u],
                        clip[3u][3u] + clip[3u][0u]};
        // get A,B,C,D for bottom plane
        _frustum[2u] = {clip[0u][3u] + clip[0u][1u], clip[1u][3u] + clip[1u][1u], clip[2u][3u] + clip[2u][1u],
                        clip[3u][3u] + clip[3u][1u]};
        // get A,B,C,D for top plane
        _frustum[3u] = {clip[0u][3u] - clip[0u][1u], clip[1u][3u] - clip[1u][1u], clip[2u][3u] - clip[2u][1u],
                        clip[3u][3u] - clip[3u][1u]};
        // get A,B,C,D for back plane
        _frustum[4u] = {clip[0u][3u] - clip[0u][2u], clip[1u][3u] - clip[1u][2u], clip[2u][3u] - clip[2u][2u],
                        clip[3u][3u] - clip[3u][2u]};
        // get A,B,C,D for front plane
        _frustum[5u] = {clip[0u][3u] + clip[0u][2u], clip[1u][3u] + clip[1u][2u], clip[2u][3u] + clip[2u][2u],
                        clip[3u][3u] + clip[3u][2u]};

        // normalize planes:
        for (auto & plane : _frustum) { // bring the equations of the planes to normal form
			// plane /= std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
            plane *= inv_sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
		}
    }
    
    bool Frustum::checkPoint(const vec3f & p) const noexcept {
        for (const auto & plane : _frustum) {
			if (glm::dot(plane, vec4f(p, 1.0f)) < 0.0f) {
				return false;
			}
		}
        return true;
    }

    bool Frustum::checkSphere(const vec3f & p, float r) const noexcept {
		for (const auto & plane : _frustum) {
			if (glm::dot(plane, vec4f(p, 1.0f)) < -r) { // the center of the sphere is further from the plane than its radius => the entire sphere is outside
				return false;
			}
		}
        return true;
    }

    bool Frustum::checkCube(const vec3f & min, const vec3f & max) const noexcept {
        // https://gamedev.ru/code/articles/FrustumCulling
		for (const auto & plane : _frustum) {
			const auto d = std::max(min.x * plane.x, max.x * plane.x)
				+ std::max(min.y * plane.y, max.y * plane.y)
				+ std::max(min.z * plane.z, max.z * plane.z)
				+ plane.w;

            if (d <= 0.0f) {
                return false;
            }
		}

        return true;
    }

    bool Frustum::checkBounds(const Bounds & bounds) const noexcept {
        switch (bounds.type) {
        case scene::Bounds::Type::eCube :
            return checkCube(bounds.volume.cube.corner0, bounds.volume.cube.corner1);
        case scene::Bounds::Type::eSphere :
            return checkSphere(bounds.volume.sphere.center, bounds.volume.sphere.radius);
        default:
            return true;
        }
        return true;
    }
}