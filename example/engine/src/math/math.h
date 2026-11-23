#pragma once

// glm options
#define SIMD_MATH // has positive effect on scenes with a large number of objects

#define GLM_FORCE_INLINE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_CXX20
#define GLM_FORCE_MESSAGES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#ifdef SIMD_MATH
// glm options for enable vectorisation
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#undef GLM_FORCE_XYZW_ONLY // do not enable it - it will break SIMD.
#else
#define GLM_FORCE_XYZW_ONLY
#endif
// glm options

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cmath>
#include <numbers>

namespace engine {
    
    template<typename T>
    concept is_number = std::integral<T> || std::floating_point<T>;

    template <typename T>
    constexpr auto pi_v = std::numbers::pi_v<T>;

    constexpr auto pi = pi_v<float>;
    constexpr auto pi2 = pi / 2.0f;

    namespace math = glm;

    template <uint8_t d, typename T>
    using vec = glm::vec<d, T, glm::defaultp>;

    template <typename T>
    using vec2 = vec<2, T>;

    template <typename T>
    using vec3 = vec<3, T>;

    template <typename T>
    using vec4 = vec<4, T>;

    template <typename T>
    using mat2 = glm::mat<2, 2, T, glm::defaultp>;

    template <typename T>
    using mat2x3 = glm::mat<2, 3, T, glm::defaultp>;

    template <typename T>
    using mat2x4 = glm::mat<2, 4, T, glm::defaultp>;

    template <typename T>
    using mat3 = glm::mat<3, 3, T, glm::defaultp>;

    template <typename T>
    using mat3x2 = glm::mat<3, 2, T, glm::defaultp>;

    template <typename T>
    using mat3x4 = glm::mat<3, 4, T, glm::defaultp>;

    template <typename T>
    using mat4 = glm::mat<4, 4, T, glm::defaultp>;

    template <typename T>
    using mat4x2 = glm::mat<4, 2, T, glm::defaultp>;

    template <typename T>
    using mat4x3 = glm::mat<4, 3, T, glm::defaultp>;

    template <typename T>
    using quat = glm::qua<T, glm::defaultp>;

    using vec2f = vec2<float>;
    using vec3f = vec3<float>;
    using vec4f = vec4<float>;
    using mat2f = mat2<float>;
    using mat2x3f = mat2x3<float>;
    using mat2x4f = mat2x4<float>;
    using mat3f = mat3<float>;
    using mat3x2f = mat3x2<float>;
    using mat3x4f = mat3x4<float>;
    using mat4f = mat4<float>;
    using mat4x2f = mat4x2<float>;
    using mat4x3f = mat4x3<float>;
    using quatf = quat<float>;

    using vec2d = vec2<double>;
    using vec3d = vec3<double>;
    using vec4d = vec4<double>;
    using mat2d = mat2<double>;
    using mat2x3d = mat2x3<double>;
    using mat2x4d = mat2x4<double>;
    using mat3d = mat3<double>;
    using mat3x2d = mat3x2<double>;
    using mat3x4d = mat3x4<double>;
    using mat4d = mat4<double>;
    using mat4x2d = mat4x2<double>;
    using mat4x3d = mat4x3<double>;
    using quatd = quat<double>;

    using vec2i = vec2<int32_t>;
    using vec3i = vec3<int32_t>;
    using vec4i = vec4<int32_t>;
    using mat2i = mat2<int32_t>;
    using mat3i = mat3<int32_t>;
    using mat4i = mat4<int32_t>;
    using quati = quat<int32_t>;

    using vec2u = vec2<uint32_t>;
    using vec3u = vec3<uint32_t>;
    using vec4u = vec4<uint32_t>;
    using mat2u = mat2<uint32_t>;
    using mat3u = mat3<uint32_t>;
    using mat4u = mat4<uint32_t>;
    using quatu = quat<uint32_t>;

    inline static const mat4f emptyMatrix   = mat4f(1.0f);
    inline static const quatf emptyQuat     = quatf(0.0f, 0.0f, 0.0f, 0.0f);
    inline static const vec2f emptyVec2     = vec2f(0.0f, 0.0f);
    inline static const vec3f emptyVec3     = vec3f(0.0f, 0.0f, 0.0f);
    inline static const vec3f unitVec3      = vec3f(1.0f, 1.0f, 1.0f);

    inline uint16_t to_float16(float value) noexcept {
        return glm::packHalf1x16(value);
    }

    template <typename T>
    inline mat4<T> make_matrix(T scale) noexcept {
        return {scale, T(0), T(0), T(0),
                T(0), scale, T(0), T(0),
                T(0), T(0), scale, T(0),
                T(0), T(0), T(0), T(1)};
    }

    template <typename T>
    inline mat4<T> make_matrix(const vec3<T> & scale) noexcept {
        return {scale.x, T(0), T(0), T(0),
                T(0), scale.y, T(0), T(0),
                T(0), T(0), scale.z, T(0),
                T(0), T(0), T(0), T(1)};
    }

    template <typename T>
    inline std::decay_t<T> step(T edge, T x) noexcept {
        return T{x >= edge};
    }

    template <typename T>
    inline std::decay_t<T> smoothstep(T && edge0, T && edge1, T && x) noexcept {
        std::decay_t<T> t;
        t = std::clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
        return t * t * (T(3) - T(2) * t);
    }

    inline float inv_sqrt(const float x) noexcept { // "инверсный(1.0f / sqrt) квадратный корень Кармака"
        const float xhalf = 0.5f * x;
        union {
            float x = 0.0f;
            int32_t i;
        } u;

        u.x = x;
        u.i = 0x5f3759df - (u.i >> 1);
        return u.x * (1.5f - xhalf * u.x * u.x);
    }

    template <typename T, typename U>
    inline T div_ceil(T numerator, U denominator) noexcept {
        return (numerator + denominator - 1u) / denominator;
    }

    inline bool is_pow2(uint32_t n) noexcept {
        return !(n & (n - 1u));
    }

    inline uint32_t next_pow2(uint32_t v) noexcept {
        if (v == 0u) return 1u;
        --v;
        v |= v >> 1u;
        v |= v >> 2u;
        v |= v >> 4u;
        v |= v >> 8u;
        v |= v >> 16u;
        ++v;
        return v;
    }

    template<typename T>
    inline int32_t sign(T && val) noexcept {
        return (T(0) < val) - (val < T(0));
    }

    template<typename T>
    inline T fract(T x) noexcept {
        return x - std::floor(x);
    }

    template<typename V>
    inline bool compare(const V & v1, const V & v2, typename V::value_type eps) noexcept {
        for (typename V::length_type i = 0U; i < V::length(); ++i) {
            if (std::abs(v1[i] - v2[i]) > eps) {
                return true;
            }
        }
        return false;
    }

    template <uint8_t d, is_number T = float>
    struct ray {
        vec<d, T> origin;
        vec<d, T> direction;
    };

    template<uint8_t d, is_number T = float>
    struct interval {
        vec<d, T> begin;
        vec<d, T> end;
    };

    using ray2f = ray<2u, float>;
    using ray3f = ray<3u, float>;
    using ray2d = ray<2u, double>;
    using ray3d = ray<3u, double>;
    using ray2i = ray<2u, int32_t>;
    using ray3i = ray<3u, int32_t>;
    using ray2u = ray<2u, uint32_t>;
    using ray3u = ray<3u, uint32_t>;

    using interval2f = interval<2u, float>;
    using interval3f = interval<3u, float>;
    using interval2d = interval<2u, double>;
    using interval3d = interval<3u, double>;
    using interval2i = interval<2u, int32_t>;
    using interval3i = interval<3u, int32_t>;
    using interval2u = interval<2u, uint32_t>;
    using interval3u = interval<3u, uint32_t>;

    inline vec2f ndcToScreen(vec2f p, vec2f size) noexcept {
        return (p * 0.5f + 0.5f) * size;
    } 

    inline vec2f screenToNdc(vec2f p, vec2f size) noexcept {
        return (2.0f * p / size) - 1.0f;
    } 

    inline vec2f worldToNdc(const mat4f & viewProjection, const vec3f & p) noexcept {
		vec4f v = viewProjection * vec4f(p, 1.0f);
		v /= v.w;
		return v;
	}

	inline interval<3u, float> ndcToWorld(const mat4f & invViewProjection, vec2f p) noexcept {
		vec4f v{p.x, p.y, 0.0f, 1.0f};

		vec4f sp = invViewProjection * v;
		sp /= sp.w;

		v.z = 1.0f;

		vec4f fp = invViewProjection * v;
		fp /= fp.w;

		return {sp, fp};
	}

    enum class RotationsOrder : uint8_t {
        eXYZ = 0u,
        eXZY = 1u,
        eYXZ = 2u,
        eYZX = 3u,
        eZXY = 4u,
        eZYX = 5u
    };

    inline void rotate_matrix_x(mat4f & m, float angle) noexcept {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const vec4f tmpV = m[1] * c + m[2] * s;
        m[2] = -m[1] * s + m[2] * c;
        m[1] = tmpV;
    }

    inline void rotate_matrix_y(mat4f & m, float angle) noexcept {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const vec4f tmpV = m[2] * c + m[0] * s;
        m[0] = -m[2] * s + m[0] * c;
        m[2] = tmpV;
    }

    inline void rotate_matrix_z(mat4f & m, float angle) noexcept {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const vec4f tmpV = m[0] * c + m[1] * s;
        m[1] = -m[0] * s + m[1] * c;
        m[0] = tmpV;
    }

    inline void direct_matrix_yz(mat4f & m, float c, float s) noexcept {
        const vec4f tmpV = m[1] * c + m[2] * s;
        m[2] = -m[1] * s + m[2] * c;
        m[1] = tmpV;
    }

    inline void direct_matrix_xz(mat4f & m, float c, float s) noexcept {
        const vec4f tmpV = m[2] * c + m[0] * s;
        m[0] = -m[2] * s + m[0] * c;
        m[2] = tmpV;
    }

    inline void direct_matrix_xy(mat4f & m, float c, float s) noexcept {
        const vec4f tmpV = m[0] * c + m[1] * s;
        m[1] = -m[0] * s + m[1] * c;
        m[0] = tmpV;
    }

    inline void scale_matrix(mat4f & matrix, const vec3f & scale) noexcept {
        matrix[0] *= scale[0];
        matrix[1] *= scale[1];
        matrix[2] *= scale[2];
    }

    inline void translate_matrix_to(mat4f & matrix, const vec3f & translate) noexcept {
        matrix[3][0] = translate.x;
        matrix[3][1] = translate.y;
        matrix[3][2] = translate.z;
    }

    inline void translate_matrix(mat4f & matrix, const vec3f & translate) noexcept {
        matrix[3] = matrix[0] * translate[0] + matrix[1] * translate[1] + matrix[2] * translate[2] + matrix[3];
    }

    inline void rotate_matrix_xyz(mat4f & transform, const vec3f & v) noexcept {
        rotate_matrix_x(transform, v.x);
        rotate_matrix_y(transform, v.y);
        rotate_matrix_z(transform, v.z);
    }

    inline void rotate_matrix_xzy(mat4f & transform, const vec3f & v) noexcept {
        rotate_matrix_x(transform, v.x);
        rotate_matrix_z(transform, v.z);
        rotate_matrix_y(transform, v.y);
    }

    inline void rotate_matrix_yxz(mat4f & transform, const vec3f & v) noexcept {
        rotate_matrix_y(transform, v.y);
        rotate_matrix_x(transform, v.x);
        rotate_matrix_z(transform, v.z);
    }

    inline void rotate_matrix_yzx(mat4f & transform, const vec3f & v) noexcept {
        rotate_matrix_y(transform, v.y);
        rotate_matrix_z(transform, v.z);
        rotate_matrix_x(transform, v.x);
    }

    inline void rotate_matrix_zxy(mat4f & transform, const vec3f & v) noexcept {
        rotate_matrix_z(transform, v.z);
        rotate_matrix_x(transform, v.x);
        rotate_matrix_y(transform, v.y);
    }

    inline void rotate_matrix_zyx(mat4f & transform, const vec3f & v) noexcept {
        rotate_matrix_z(transform, v.z);
        rotate_matrix_y(transform, v.y);
        rotate_matrix_x(transform, v.x);
    }

    inline void rotate_matrix_by_order(mat4f & transform, const vec3f & v, RotationsOrder r) noexcept {
        switch (r) {
            case RotationsOrder::eXYZ:
                rotate_matrix_xyz(transform, v);
                break;
            case RotationsOrder::eXZY:
                rotate_matrix_xzy(transform, v);
                break;
            case RotationsOrder::eYXZ:
                rotate_matrix_yxz(transform, v);
                break;
            case RotationsOrder::eYZX:
                rotate_matrix_yzx(transform, v);
                break;
            case RotationsOrder::eZXY:
                rotate_matrix_zxy(transform, v);
                break;
            case RotationsOrder::eZYX:
                rotate_matrix_zyx(transform, v);
                break;
            default:
                break;
        }
    }

    inline void decompose_matrix(
        const mat4f & m,
        vec3f & s,
        quatf & r,
        vec3f & t
    ) noexcept {
        s.x = glm::length(vec3f(m[0]));
        s.y = glm::length(vec3f(m[1]));
        s.z = glm::length(vec3f(m[2]));
        // glm::sign(m[0][0]) - и аналогичные, для знака скелинга?

        if (s == emptyVec3) {
            r = emptyQuat;
            t = emptyVec3;
        } else {
            const mat4f m1 = glm::scale(m, 1.0f / s);
            r = glm::normalize(glm::quat_cast(m1));
            t.x = m1[3].x;
            t.y = m1[3].y;
            t.z = m1[3].z;
        }
    }

    inline mat4f mat4_from_quat(const quatf & q) noexcept {
        mat4f result;

        const float x2(q.x + q.x);
        const float y2(q.y + q.y);
        const float z2(q.z + q.z);

        const float wx2(q.w * x2); const float wy2(q.w * y2); const float wz2(q.w * z2);
        const float xx2(q.x * x2); const float xy2(q.x * y2); const float xz2(q.x * z2);
        const float yy2(q.y * y2); const float yz2(q.y * z2); const float zz2(q.z * z2);

        result[0u][0u] = 1.0f - (yy2 + zz2);
        result[0u][1u] = xy2 + wz2;
        result[0u][2u] = xz2 - wy2;
        result[0u][3u] = 0.0f;

        result[1u][0u] = xy2 - wz2;
        result[1u][1u] = 1.0f - (xx2 + zz2);
        result[1u][2u] = yz2 + wx2;
        result[1u][3u] = 0.0f;

        result[2u][0u] = xz2 + wy2;
        result[2u][1u] = yz2 - wx2;
        result[2u][2u] = 1.0f - (xx2 + yy2);
        result[2u][3u] = 0.0f;

        result[3u][0u] = 0.0f;
        result[3u][1u] = 0.0f;
        result[3u][2u] = 0.0f;
        result[3u][3u] = 1.0f;

        return result;
    }
}