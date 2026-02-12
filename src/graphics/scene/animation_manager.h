#pragma once

#include "animation.h"
#include "common/hash.h"
#include "common/type_id.h"

#include "animation/mesh/skeletal_animation.h"
#include "animation/texture/frame_sequence_animation.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::scene {

class AnimationManager {
public:
    static constexpr uint16_t kUnknownAnimation = 0xff'ffu;

    template <typename T>
    const T * getAnimation(uint16_t animationId) const noexcept {
        const Animation * result = nullptr;
        switch (ctti::typeIndex<Animation, T>()) {
        case ctti::typeIndex<Animation, SkeletalAnimation>() :
            if (animationId < _skeletalAnimations.size()) {
                result = &_skeletalAnimations[animationId];
            }
            break;
        case ctti::typeIndex<Animation, FramesSequence>() :
            if (animationId < _sequences.size()) {
                result = &_sequences[animationId];
            }
            break;
        default:
            break;
        }
        return static_cast<const T*>(result);
    }

    template <typename T>
    uint16_t registerAnimation(std::string_view name, T && anim) {
        if (getAnimationIndex(name) != kUnknownAnimation) {
            // animation with name "name" already exists
            assert(false);
        }

        if constexpr (ctti::typeIndex<Animation, T>() == ctti::typeIndex<Animation, SkeletalAnimation>())  {
            auto const result = static_cast<uint16_t>(_skeletalAnimations.size());
            _skeletalAnimations.emplace_back(std::forward<SkeletalAnimation>(anim));
            _animationNames.emplace(std::string{name}, std::pair{static_cast<uint16_t>(ctti::typeIndex<Animation, SkeletalAnimation>()), result});
            return result;
        }

        if constexpr (ctti::typeIndex<Animation, T>() == ctti::typeIndex<Animation, FramesSequence>()) {
            auto const result = static_cast<uint16_t>(_sequences.size());
            _sequences.emplace_back(std::forward<FramesSequence>(anim));
            _animationNames.emplace(std::string{name}, std::pair{static_cast<uint16_t>(ctti::typeIndex<Animation, FramesSequence>()), result});
            return result;
        }
    }

    void unregisterAnimation(std::string_view name) noexcept {
        if (auto it = _animationNames.find(name); it != _animationNames.end()) {
            // todo
            return;
        }
        // animation with name "name" is no exists
        return;
    }

    uint16_t getAnimationIndex(std::string_view name) const noexcept {
        if (auto it = _animationNames.find(name); it != _animationNames.end()) {
            return it->second.second;
        }
        return kUnknownAnimation;
    }

    void destroy() noexcept;

private:
    using AnimationNamesMap = std::unordered_map<std::string, std::pair<uint16_t, uint16_t>, hash::String_hash, std::equal_to<>>;
    // use vector of vectors?
    std::vector<SkeletalAnimation> _skeletalAnimations;
    std::vector<FramesSequence> _sequences;
    AnimationNamesMap _animationNames;
};

}