#include "skeletal_animation.h"
#include "graphics/scene/node.h"
#include "graphics/scene/objects/mesh.h"
#include "graphics/mesh/gltf_loader.h"

#include <algorithm>
#include <cassert>

namespace engine::scene {

    void SkeletalAnimator::resize(uint32_t count) {
        _transforms.resize(count);
        for (auto & t : _transforms) {
            t.mask = TransformChangeFlags::eNone;
        }
    }

    void SkeletalAnimator::enlarge2(uint32_t count) {
        if (count <= _transforms.size()) {
            return;
        }

        resize(count);
    }

    void SkeletalAnimator::update2(float time, const SkeletalAnimation & animation) noexcept {
        using namespace graphics::models;
        constexpr float epsilon = 1e-5f;

        for (auto & t : _transforms) {
            t.mask = TransformChangeFlags::eNone;
        }

        const auto & anim = animation.getAnimation();
        time = glm::mod(time, anim.duration);

        for (const auto & channel : anim.channels) {
            if (channel.sampler == kInvalidValue || channel.target_node == kInvalidValue) {
                continue;
            }

            const auto & sampler = anim.samplers[channel.sampler];
            const auto & inputs = sampler.inputs;

            // glTF requires sampler inputs to be strictly increasing, so the key interval can be
            // found with a binary search
            if (inputs.size() < 2u) {
                continue;
            }

            const auto upper = std::upper_bound(inputs.begin(), inputs.end(), time);
            if (upper == inputs.begin() || upper == inputs.end()) {
                // time is outside of the sampler range: skip channel
                continue;
            }

            const auto i = static_cast<size_t>(upper - inputs.begin()) - 1u;
            const auto t0 = inputs[i];
            const auto t1 = inputs[i + 1u];
            const auto & v0 = sampler.outputs[i];

            auto & target = _transforms[channel.target_node];

            switch (sampler.interpolation) {
            case Interpolation::eLinear: {
                const auto & v1 = sampler.outputs[i + 1u];
                const auto mix = (time - t0) / (t1 - t0);
                switch (channel.path) {
                case AnimationChannelPath::eTranslation: {
                    if (glm::all(glm::epsilonEqual(v0, v1, epsilon))) {
                        target.position = v0;
                    } else {
                        target.position = glm::mix(v0, v1, mix);
                    }
                    target.mask = target.mask | TransformChangeFlags::ePosition;
                }
                    break;
                case AnimationChannelPath::eRotation: {
                    if (glm::all(glm::epsilonEqual(v0, v1, epsilon))) {
                        target.rotation = {v0.w, v0.x, v0.y, v0.z};
                    } else {
                        const quatf q0(v0.w, v0.x, v0.y, v0.z);
                        const quatf q1(v1.w, v1.x, v1.y, v1.z);
                        // is normalize necessary?
                        target.rotation = glm::normalize(glm::slerp(q0, q1, mix));
                    }
                    target.mask = target.mask | TransformChangeFlags::eRotation;
                }
                    break;
                case AnimationChannelPath::eScale: {
                    if (glm::all(glm::epsilonEqual(v0, v1, epsilon))) {
                        target.scale = v0;
                    } else {
                        target.scale = glm::mix(v0, v1, mix);
                    }
                    target.mask = target.mask | TransformChangeFlags::eScale;
                }
                    break;
                }
            }
                break;
            case Interpolation::eStep: {
                switch (channel.path) {
                case AnimationChannelPath::eTranslation:
                    target.position = v0;
                    target.mask = target.mask | TransformChangeFlags::ePosition;
                    break;
                case AnimationChannelPath::eRotation:
                    target.rotation = {v0.w, v0.x, v0.y, v0.z};
                    target.mask = target.mask | TransformChangeFlags::eRotation;
                    break;
                case AnimationChannelPath::eScale:
                    target.scale = v0;
                    target.mask = target.mask | TransformChangeFlags::eScale;
                    break;
                }
            }
                break;
            case Interpolation::eCubicSpline:
            {
                assert(false);
                const auto & v1 = sampler.outputs[i + 1u];
                const auto mix = (time - t0) / (t1 - t0);

                const float t2 = mix * mix;
                const float t3 = t2 * mix;

                // switch (channel.path) {
                // case AnimationChannelPath::eTranslation: {
                //     if (glm::all(glm::epsilonEqual(v0, v1, epsilon))) {
                //         target.position = v0;
                //     } else {
                //         target.position = glm::mix(v0, v1, mix);
                //     }
                //     target.mask = target.mask | TransformChangeFlags::ePosition;
                // }
                //     break;
                // case AnimationChannelPath::eRotation: {
                //     if (glm::all(glm::epsilonEqual(v0, v1, epsilon))) {
                //         target.rotation = {v0.w, v0.x, v0.y, v0.z};
                //     } else {
                //         const quatf q0(v0.w, v0.x, v0.y, v0.z);
                //         const quatf q1(v1.w, v1.x, v1.y, v1.z);
                //         // is normalize necessary?
                //         target.rotation = glm::normalize(glm::slerp(q0, q1, mix));
                //     }
                //     target.mask = target.mask | TransformChangeFlags::eRotation;
                // }
                //     break;
                // case AnimationChannelPath::eScale: {
                //     if (glm::all(glm::epsilonEqual(v0, v1, epsilon))) {
                //         target.scale = v0;
                //     } else {
                //         target.scale = glm::mix(v0, v1, mix);
                //     }
                //     target.mask = target.mask | TransformChangeFlags::eScale;
                // }
                //     break;
                // }


            }
                break;
            }
        }
    }

    void SkeletalAnimator::mix(const SkeletalAnimator & animator0, const SkeletalAnimator & animator1, 
        const graphics::models::MeshData & meshData, float mix) noexcept {
        const SkeletalAnimator * minAnimator = &animator0;
        const SkeletalAnimator * maxAnimator = &animator1;

        auto min = minAnimator->_transforms.size();
        auto max = maxAnimator->_transforms.size();
        if (max < min) {
            std::swap(minAnimator, maxAnimator);
            std::swap(min, max);
        }

        for (size_t i = 0u; i < min; ++i) {
            auto & tr = _transforms[i];
            const auto & tr0 = animator0._transforms[i];
            const auto & tr1 = animator1._transforms[i];
            const auto & defaultNode = meshData.nodes[i];

            const auto defaultScale = vec3f{defaultNode.scale.x, defaultNode.scale.y, defaultNode.scale.z};
            const auto defaultPosition = vec3f{defaultNode.translation.x, defaultNode.translation.y, defaultNode.translation.z};
            const auto defaultQuat = quatf{defaultNode.rotation.w, defaultNode.rotation.x, defaultNode.rotation.y, defaultNode.rotation.z};

            tr.mask = tr0.mask | tr1.mask;

            if (((tr0.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone)
                && ((tr1.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone)) {
                tr.scale = glm::mix(tr0.scale, tr1.scale, mix);
            } else if ((tr0.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone) {
                tr.scale = glm::mix(tr0.scale, defaultScale, mix);
            } else if ((tr1.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone) {
                tr.scale = glm::mix(defaultScale, tr1.scale, mix);
            }

            if (((tr0.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone)
                && ((tr1.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone)) {
                tr.rotation = glm::normalize(glm::slerp(tr0.rotation, tr1.rotation, mix));
            } else if ((tr0.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone) {
                tr.rotation = glm::normalize(glm::slerp(tr0.rotation, defaultQuat, mix));
            } else if ((tr1.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone) {
                tr.rotation = glm::normalize(glm::slerp(defaultQuat, tr1.rotation, mix));
            }

            if (((tr0.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone)
                && ((tr1.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone)) {
                tr.position = glm::mix(tr0.position, tr1.position, mix);
            } else if ((tr0.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone) {
                tr.position = glm::mix(tr0.position, defaultPosition, mix);
            } else if ((tr1.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone) {
                tr.position = glm::mix(defaultPosition, tr1.position, mix);
            }
        }

        for (size_t i = min; i < max; ++i) {
            auto & tr = _transforms[i];
            auto const & tr0 = maxAnimator->_transforms[i];

            tr.mask = tr0.mask;
            if ((tr.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone) {
                tr.scale = tr0.scale;
            }
            if ((tr0.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone) {
                tr.rotation = tr0.rotation;
            }
            if ((tr0.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone) {
                tr.position = tr0.position;
            }
        }
    }
    
    void SkeletalAnimator::apply2(Skeleton & skeleton) const noexcept {
        uint32_t i = 0U;
        auto const maxNode = skeleton.nodesCount();
        for (auto const & transform : _transforms) {
            if (i >= maxNode) {
                break;
            }

            auto const & defaultNode = skeleton.meshData().nodes[i];
            const auto defaultScale = vec3f{defaultNode.scale.x, defaultNode.scale.y, defaultNode.scale.z};
            const auto defaultPosition = vec3f{defaultNode.translation.x, defaultNode.translation.y, defaultNode.translation.z};
            const auto defaultQuat = quatf{defaultNode.rotation.w, defaultNode.rotation.x, defaultNode.rotation.y, defaultNode.rotation.z};

            auto * target = skeleton.node(i);
            if ((transform.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone) {
                target->setScale(transform.scale);
            } else {
                target->setScale(defaultScale);
            }

            if ((transform.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone) {
                target->setRotation(transform.rotation);
            } else {
                target->setRotation(defaultQuat);
            }

            if ((transform.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone) {
                target->setPosition(transform.position);
            } else {
                target->setPosition(defaultPosition);
            }
            ++i;
        }
    }

    void SkeletalAnimator::apply2(Skeleton & skeleton, float mix) const noexcept {
        uint32_t i = 0U;
        auto const maxNode = skeleton.nodesCount();
        for (auto const & transform : _transforms) {
            if (i >= maxNode) {
                break;
            }

            auto const & defaultNode = skeleton.meshData().nodes[i];
            const auto defaultScale = vec3f{defaultNode.scale.x, defaultNode.scale.y, defaultNode.scale.z};
            const auto defaultPosition = vec3f{defaultNode.translation.x, defaultNode.translation.y, defaultNode.translation.z};
            const auto defaultQuat = quatf{defaultNode.rotation.w, defaultNode.rotation.x, defaultNode.rotation.y, defaultNode.rotation.z};

            auto * target = skeleton.node(i);
            if ((transform.mask & TransformChangeFlags::eScale) != TransformChangeFlags::eNone) {
                target->setScale(glm::mix(target->getScale(), transform.scale, mix));
            } else {
                target->setScale(glm::mix(target->getScale(), defaultScale, mix));
            }

            if ((transform.mask & TransformChangeFlags::eRotation) != TransformChangeFlags::eNone) {
                target->setRotation(glm::normalize(glm::slerp(target->getRotation(), transform.rotation, mix)));
            } else {
                target->setRotation(glm::normalize(glm::slerp(target->getRotation(), defaultQuat, mix)));
            }

            if ((transform.mask & TransformChangeFlags::ePosition) != TransformChangeFlags::eNone) {
                target->setPosition(glm::mix(target->getPosition(), transform.position, mix));
            } else {
                target->setPosition(glm::mix(target->getPosition(), defaultPosition, mix));
            }
            ++i;
        }
    }

}