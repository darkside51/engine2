#pragma once

#include "math/math.h"
#include "utils/intrusive_list.h"

#include <cstdint>

namespace engine::scene {
    // constexpr uint32_t kEmptyAnimationDataIndex = 0xff'ff'ff'ffu;

    struct Node;
    struct AnimationInfoChain;

    struct Object : IntrusiveListNode<Object> {
        using Iterator = IntrusiveList<Object>::Iterator;
        Node * node = nullptr;
        AnimationInfoChain * animInfo = nullptr;
       
        uint16_t typeIndex = 0u;
        // uint16_t layer = 0u;
        int16_t order = 0u;
        // uint32_t animationDataIndex = kEmptyAnimationDataIndex; 

        Object(uint16_t typeIndex) : typeIndex(typeIndex) {}
        /*virtual*/ ~Object() = default; // call typed, virtual not need

        // void setLayer(uint16_t l) noexcept { layer = l; }
        // uint16_t getLayer() const noexcept { return layer; }

        void setOrder(int16_t o) noexcept { order = o; }
        int16_t getOrder() const noexcept { return order; }

        void addAnimation(AnimationInfoChain * anim) noexcept;
        bool removeAnimation(AnimationInfoChain * anim) noexcept;
        
        bool cmp(const Object &) const noexcept { return false; }
    };

    struct Dummy : Object {
        explicit Dummy();
        ~Dummy() /*override*/ = default;
    };

}