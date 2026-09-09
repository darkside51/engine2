#include "object.h"
#include "common/type_id.h"
#include "graphics/scene/register.h"

#include <chrono>

namespace engine::scene {

    // void AttachmentObject::addAnimation(AnimationInfoChain * anim) noexcept {
    //     if (animInfo == nullptr) {
    //         animInfo = anim;
    //     } else {
    //         auto * freeInfo = animInfo;
    //         while (freeInfo->next) {
    //             freeInfo = freeInfo->next;
    //         }
    //         freeInfo->next = anim;
    //     }
    // }

    // bool AttachmentObject::removeAnimation(AnimationInfoChain * anim) noexcept { // extract
    //     auto * freeInfo = animInfo;
    //     while (freeInfo) {
    //         if (freeInfo == anim) {
    //             if (freeInfo->prev) {
    //                 freeInfo->prev->next = freeInfo->next;
    //             }
    //             if (freeInfo->next) {
    //                 freeInfo->next->prev = freeInfo->prev;
    //             }
    //             return true;
    //         }
    //         freeInfo = freeInfo->next;
    //     }
    //     return false;
    // }

    bool AttachmentObject::removeAnimation() noexcept { // extract
        animInfo = nullptr;
        return true;
    }

    AnimationDescription * AttachmentObject::setAnimation(AnimationDescription * anim) noexcept {
        if (animInfo == nullptr) {
            animInfo = anim;
            return nullptr;
        } else {
            auto * result = animInfo;
            animInfo = anim;
            return result;
        }
    }

    void AttachmentObject::setAnimation(const AnimationInfo & anim, float mix) noexcept {
        if (animInfo == nullptr) {
            return;
        }

        animInfo->target = anim;
        animInfo->mix = mix;
    }

    void AttachmentObject::setAnimation(uint16_t animationId, float mix) noexcept {
        if (animInfo == nullptr) {
            return;
        }

        animInfo->target.animationId = animationId;
        animInfo->target.animationType = animInfo->current.animationType;

        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<float> nowDuration = now.time_since_epoch();
        animInfo->target.startTime = nowDuration.count();
        animInfo->mix = mix;
    }

    uint16_t AttachmentObject::getAnimationId() const noexcept {
        return animInfo ? animInfo->current.animationId : kUndefinedAnim;
    }

    float AttachmentObject::getAnimMix() const noexcept {
        return animInfo ? animInfo->mix : 0.0f;
    }

    Dummy::Dummy() : AttachmentObject(ctti::typeIndex<AttachmentObject, Dummy>()) {}
}