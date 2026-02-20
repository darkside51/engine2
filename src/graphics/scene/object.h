#pragma once

// #include "math/math.h"
// #include "utils/intrusive_list.h"

#include <cstdint>

namespace engine::scene {
    // constexpr uint32_t kEmptyAnimationDataIndex = 0xff'ff'ff'ffu;

    struct Node;
    struct AnimationInfoChain;

    namespace AttachmentFlagBits {
        constexpr uint8_t kHiddenBit = 0u;
    }

    struct AttachmentObject /*: IntrusiveListNode<Object>*/ {
        struct Flags {
            uint8_t flags = 0u;

            inline bool isHidden() const noexcept {
                return flags & (1u << AttachmentFlagBits::kHiddenBit);
            }

            inline void setHidden(bool hidden) noexcept {
                hidden ? flags |= (1u << AttachmentFlagBits::kHiddenBit) : 
                         flags &= ~(1u << AttachmentFlagBits::kHiddenBit);
            }
        };

        Node * node = nullptr;
        AnimationInfoChain * animInfo = nullptr;
        AttachmentObject * next = nullptr;
       
        uint16_t typeIndex = 0u;
        // uint16_t layer = 0u;
        int16_t order = 0u; 

        Flags flags{};

        AttachmentObject(uint16_t typeIndex) : typeIndex(typeIndex) {}
        /*virtual*/ ~AttachmentObject() = default; // call typed, virtual not need

        // void setLayer(uint16_t l) noexcept { layer = l; }
        // uint16_t getLayer() const noexcept { return layer; }

        void setOrder(int16_t o) noexcept { order = o; }
        int16_t getOrder() const noexcept { return order; }

        void addAnimation(AnimationInfoChain * anim) noexcept;
        bool removeAnimation(AnimationInfoChain * anim) noexcept;
        AnimationInfoChain * setAnimation(AnimationInfoChain * anim) noexcept;

        bool isHidden() const noexcept {
            return flags.isHidden();
        }

        void setHidden(bool hidden) noexcept {
            flags.setHidden(hidden);
        }
        
        bool cmp(const AttachmentObject &) const noexcept { return false; }
    };

    struct Dummy : AttachmentObject {
        explicit Dummy();
        ~Dummy() /*override*/ = default;
    };

}