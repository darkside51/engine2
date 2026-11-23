#pragma once

// #include "math/math.h"
// #include "utils/intrusive_list.h"

#include <cstdint>

namespace engine::scene {
    // constexpr uint32_t kEmptyAnimationDataIndex = 0xff'ff'ff'ffu;

    struct Node;
    struct AnimationDescription;
    struct AnimationInfo;

    namespace AttachmentFlagBits {
        constexpr uint8_t kHiddenBit = 0u;
        constexpr uint8_t kOutlineBit = 1u;
        constexpr uint8_t kColorIdBit = 2u;
    }

    struct AttachmentObject /*: IntrusiveListNode<Object>*/ {
        inline static uint32_t gUID = 0xff'ff'ff'ffu; // max value, first add set id 0u
        struct Flags {
            uint8_t flags = 0u;

            inline bool isHidden() const noexcept {
                return flags & (1u << AttachmentFlagBits::kHiddenBit);
            }

            inline void setHidden(bool hidden) noexcept {
                hidden ? flags |= (1u << AttachmentFlagBits::kHiddenBit) : 
                         flags &= ~(1u << AttachmentFlagBits::kHiddenBit);
            }

            inline bool isOutlined() const noexcept {
                return flags & (1u << AttachmentFlagBits::kOutlineBit);
            }

            inline void setOutlined(bool value) noexcept {
                value ? flags |= (1u << AttachmentFlagBits::kOutlineBit) : 
                        flags &= ~(1u << AttachmentFlagBits::kOutlineBit);
            }

            inline bool isEnableColorId() const noexcept {
                return flags & (1u << AttachmentFlagBits::kColorIdBit);
            }

            inline void setEnableColorId(bool value) noexcept {
                value ? flags |= (1u << AttachmentFlagBits::kColorIdBit) : 
                        flags &= ~(1u << AttachmentFlagBits::kColorIdBit);
            }
        };

        Node * node = nullptr;
        AnimationDescription * animInfo = nullptr;
        AttachmentObject * next = nullptr;
       
        uint32_t id = 0u;
        uint16_t typeIndex = 0u;
        // uint16_t layer = 0u;
        int16_t order = 0u; 

        Flags flags{};
        uint8_t animationQualityFactor = 0u;
        uint8_t qualityLevelsId = 0xffu;

        AttachmentObject(uint16_t typeIndex) : id(++gUID), typeIndex(typeIndex) {}
        /*virtual*/ ~AttachmentObject() = default; // call typed, virtual not need

        // void setLayer(uint16_t l) noexcept { layer = l; }
        // uint16_t getLayer() const noexcept { return layer; }

        void setOrder(int16_t o) noexcept { order = o; }
        int16_t getOrder() const noexcept { return order; }

        // void addAnimation(AnimationInfoChain * anim) noexcept;
        // bool removeAnimation(AnimationInfoChain * anim) noexcept;
        bool removeAnimation() noexcept;
        AnimationDescription * setAnimation(AnimationDescription * anim) noexcept;
        void setAnimation(const AnimationInfo & anim, float mix = 0.0f) noexcept;
        void setAnimation(float nowTime, uint16_t animationId, float speed = 1.0f, float mix = 0.0f) noexcept;
        uint16_t getAnimationId() const noexcept;
        float getAnimMix() const noexcept;

        bool isHidden() const noexcept {
            return flags.isHidden();
        }

        void setHidden(bool hidden) noexcept {
            flags.setHidden(hidden);
        }

        bool isOutlined() const noexcept {
            return flags.isOutlined();
        }

        void setOutlined(bool value) noexcept {
            flags.setOutlined(value);
        }

        bool isEnableColorId() const noexcept {
            return flags.isEnableColorId();
        }

        void setEnableColorId(bool value) noexcept {
            flags.setEnableColorId(value);
        }
    
        bool cmp(const AttachmentObject &) const noexcept { return false; }
    };

    struct Dummy : AttachmentObject {
        explicit Dummy();
        ~Dummy() /*override*/ = default;
    };

}