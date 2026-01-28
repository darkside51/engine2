#pragma once

#include "common/type_id.h"
#include "animation.h"
#include "animation/texture/frame_sequence_animation.h"
#include "object.h"
#include "objects/mesh.h"
#include "objects/sprite.h"

namespace engine::scene {

    static consteval void registerTypes() noexcept {
        // objects
        static_assert(ctti::typeIndex<Object, Mesh>() == 0u);
        static_assert(ctti::typeIndex<Object, Dummy>() == 1u);
        static_assert(ctti::typeIndex<Object, Sprite>() == 2u);
        
        // animations
        static_assert(ctti::typeIndex<Animation, FramesSequence>() == 0u);
    }

}