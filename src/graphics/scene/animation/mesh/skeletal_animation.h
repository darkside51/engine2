#pragma once

#include "math/math.h"
#include "graphics/mesh/mesh_data.h"
#include "graphics/scene/animation.h"

#include <chrono>
#include <span>
#include <utility>
#include <vector>

namespace engine::scene {

    class Skeleton;

    class SkeletalAnimation : public Animation {
    public:
        ~SkeletalAnimation() = default;
        SkeletalAnimation(const graphics::models::MeshAnimation & animation) : _animation(&animation) {}

        const graphics::models::MeshAnimation & getAnimation() const noexcept {
            return *_animation;
        }

    private:
        const graphics::models::MeshAnimation * _animation;
    };

    enum class TransformChangeFlags : uint8_t { 
        eNone = 0u, 
        eScale = 1u, 
        eRotation = 2u, 
        ePosition = 4u
    };
    inline TransformChangeFlags operator | (TransformChangeFlags a, TransformChangeFlags b) noexcept {
        return static_cast<TransformChangeFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline TransformChangeFlags operator & (TransformChangeFlags a, TransformChangeFlags b) noexcept {
        return static_cast<TransformChangeFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    class SkeletalAnimator {
        struct Transform {
            TransformChangeFlags mask = {};
            vec3f scale = {1.0f, 1.0f, 1.0f};
            quatf rotation = {1.0f, 0.0f, 0.0f, 0.0f};
            vec3f position = {0.0f, 0.0f, 0.0f};
        };

    public:
        SkeletalAnimator() = default;
        explicit SkeletalAnimator(uint32_t count) : _transforms(count) {}
        void resize(uint32_t count);
        void enlarge2(uint32_t count);
        
        void update2(float time, const SkeletalAnimation & animation) noexcept;

        stl::span<const Transform> getTransforms() const noexcept { return _transforms; }

        // mix values from 2 animators
        void mix(const SkeletalAnimator & animator0, const SkeletalAnimator & animator1,
             const graphics::models::MeshData & meshData, float mix) noexcept;

        void apply2(Skeleton & skeleton) const noexcept;
        void apply2(Skeleton & skeleton, float mix) const noexcept;

    private:
        stl::vector<Transform> _transforms;
    };
}