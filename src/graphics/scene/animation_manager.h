#pragma once

#include "animation.h"
#include "common/hash.h"
#include "common/type_id.h"

#include "animation/texture/frame_sequence_animation.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::scene {

class AnimationManager {
public:
    template <typename T>
    const T * getAnimation(uint16_t animationId) const noexcept { 
        switch (ctti::typeIndex<Animation, T>()) {
        case ctti::typeIndex<Animation, FramesSequence>() :
            if (animationId < _sequences.size()) {
                return &_sequences[animationId];
            }
            break;
        default:
            break;
        }
        return nullptr;
    }

    template <typename T>
    uint16_t registerAnimation(std::string_view name, T && anim) {
        switch (ctti::typeIndex<Animation, T>()) {
        case ctti::typeIndex<Animation, FramesSequence>() :
        {
            auto const result = static_cast<uint16_t>(_sequences.size());
            _sequences.emplace_back(std::move(anim));
            _animationNames.emplace(std::string{name}, std::pair{static_cast<uint16_t>(ctti::typeIndex<Animation, FramesSequence>()), result});
            return result;
        }
        default:
            break;
        }
    }

    uint16_t getAnimationIndex(std::string_view name) const noexcept {
        if (auto it = _animationNames.find(name); it != _animationNames.end()) {
            return it->second.second;
        }
        return 0xff'ffu;
    }

    void destroy() noexcept;

private:
    using AnimationNamesMap = std::unordered_map<std::string, std::pair<uint16_t, uint16_t>, hash::String_hash, std::equal_to<>>;
    std::vector<FramesSequence> _sequences;
    AnimationNamesMap _animationNames;
};

}