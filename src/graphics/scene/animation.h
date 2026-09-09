#pragma once

#include <cstdint>

namespace engine::scene {

struct Animation {
    uint16_t typeIndex = 0u;
};

// struct AnimationInfo {
//     float startTime = 0.0f;
//     float weight = 1.0f;
//     uint16_t animationId;
//     uint16_t animationType;
// };

// struct AnimationInfoChain {
//     AnimationInfo info;
//     AnimationInfoChain * prev = nullptr;
//     AnimationInfoChain * next = nullptr;

//     AnimationInfoChain(float startTime, float weight, uint16_t animationId, uint16_t animationType) : 
//         info{startTime, weight, animationId, animationType} {}
// };

constexpr uint16_t kUndefinedAnim = 0xff'ffu;

enum class AnimSwitchType : uint8_t {
    eNone = 0u,
    eByTime = 1u
};

struct AnimationInfo {
    float startTime = 0.0f;
    uint16_t animationId;
    uint16_t animationType = kUndefinedAnim;
};

struct AnimationDescription {
    AnimationInfo current;
    AnimationInfo target;
    float mix = 0.0f;
    AnimSwitchType animSwitchType;

    AnimationDescription(uint16_t type, uint16_t animationId, float startTime, AnimSwitchType switchType) :
        current{startTime, animationId, type},
        animSwitchType(switchType) { }
};

}