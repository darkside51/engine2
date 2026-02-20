#pragma once

#include <cstdint>

namespace engine::scene {

struct Animation {
    uint16_t typeIndex = 0u;
};

struct AnimationInfo {
    float startTime = 0.0f;
    float sleepTime = 0.0f;
    float weight = 1.0f;
    uint16_t animationId;
    uint16_t animationType;
};

struct AnimationInfoChain {
    AnimationInfo info;
    AnimationInfoChain * prev = nullptr;
    AnimationInfoChain * next = nullptr;

    AnimationInfoChain(float startTime, float weight, uint16_t animationId, uint16_t animationType) : 
        info{startTime, 0.0f, weight, animationId, animationType} {}
};

}