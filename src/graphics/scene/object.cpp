#include "object.h"
#include "common/type_id.h"
#include "graphics/scene/register.h"

namespace engine::scene {

    void Object::addAnimation(AnimationInfoChain * anim) noexcept {
        if (animInfo == nullptr) {
            animInfo = anim;
        } else {
            auto * freeInfo = animInfo;
            while (freeInfo->next) {
                freeInfo = freeInfo->next;
            }
            freeInfo->next = anim;
        }
    }

    bool Object::removeAnimation(AnimationInfoChain * anim) noexcept { // extract
        auto * freeInfo = animInfo;
        while (freeInfo) {
            if (freeInfo == anim) {
                if (freeInfo->prev) {
                    freeInfo->prev->next = freeInfo->next;
                }
                if (freeInfo->next) {
                    freeInfo->next->prev = freeInfo->prev;
                }
                return true;
            }
            freeInfo = freeInfo->next;
        }
        return false;
    }

    AnimationInfoChain * Object::setAnimation(AnimationInfoChain * anim) noexcept {
        if (animInfo == nullptr) {
            animInfo = anim;
            return nullptr;
        } else {
            auto * result = animInfo;
            animInfo = anim;
            return result;
        }
    }

    Dummy::Dummy() : Object(ctti::typeIndex<Object, Dummy>()) {}
}