#pragma once

#include "common/type_id.h"
#include "animation.h"
#include "animation/texture/frame_sequence_animation.h"
#include "object.h"
#include "sprite/sprite.h"

namespace engine::scene {

    static consteval void registerTypes() noexcept {
        // objects
        static_assert(ctti::typeIndex<Object, Dummy>() == 0u);
        static_assert(ctti::typeIndex<Object, Sprite>() == 1u);

        // animations
        static_assert(ctti::typeIndex<Animation, FramesSequence>() == 0u);
    }

}