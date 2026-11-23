#pragma once

#include "animation.h"
#include "common/hash.h"
#include "common/type_id.h"

#include "animation/mesh/skeletal_animation.h"
#include "animation/texture/frame_sequence_animation.h"
#include "stl/unordered_map.h"
#include "stl/utility.h"
#include "stl/vector.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>

namespace engine::scene {

class AnimationManager {
public:
    static constexpr uint16_t kUnknownAnimation = 0xff'ffu;

    struct AnimationRemap {
        uint32_t type = 0xff'ff'ff'ffu;
        uint16_t from = 0xff'ffu;
        uint16_t to = 0xff'ffu;
    };

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
            _animationNames.emplace(std::string{name}, stl::pair{static_cast<uint16_t>(ctti::typeIndex<Animation, SkeletalAnimation>()), result});
            return result;
        }

        if constexpr (ctti::typeIndex<Animation, T>() == ctti::typeIndex<Animation, FramesSequence>()) {
            auto const result = static_cast<uint16_t>(_sequences.size());
            _sequences.emplace_back(std::forward<FramesSequence>(anim));
            _animationNames.emplace(std::string{name}, stl::pair{static_cast<uint16_t>(ctti::typeIndex<Animation, FramesSequence>()), result});
            return result;
        }
    }

    [[nodiscard]] AnimationRemap unregisterAnimation(std::string_view name) noexcept {
        if (auto it = _animationNames.find(name); it != _animationNames.end()) {
            const auto [type, i] = it->second;
            AnimationRemap result {
                .type = type,
                .from = 0xff'ffu,
                .to = i
            };

            switch (type) {
            case ctti::typeIndex<Animation, SkeletalAnimation>():
            {
                result.from = _skeletalAnimations.size() - 1u;
                std::swap(_skeletalAnimations[i], _skeletalAnimations.back());
                _skeletalAnimations.pop_back();
            }
                break;
            case ctti::typeIndex<Animation, FramesSequence>():
            {
                result.from = _sequences.size() - 1u;
                std::swap(_sequences[i], _sequences.back());
                _sequences.pop_back();
            }
                break;
            }

            return result;
        }

        // animation with name "name" is no exists
        return {};
    }

    uint16_t getAnimationIndex(std::string_view name) const noexcept {
        if (auto it = _animationNames.find(name); it != _animationNames.end()) {
            return it->second.second;
        }
        return kUnknownAnimation;
    }

    void destroy() noexcept;

private:
    using AnimationNamesMap = stl::unordered_map<std::string, stl::pair<uint16_t, uint16_t>, hash::String_hash, std::equal_to<>>;
    // use vector of vectors?
    stl::vector<SkeletalAnimation> _skeletalAnimations;
    stl::vector<FramesSequence> _sequences;
    AnimationNamesMap _animationNames;
};

}