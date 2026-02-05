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
        static_assert(ctti::typeIndex<AttachmentObject, Skeleton>() == 0u);
        static_assert(ctti::typeIndex<AttachmentObject, Mesh>() == 1u);
        static_assert(ctti::typeIndex<AttachmentObject, Sprite>() == 2u);
        static_assert(ctti::typeIndex<AttachmentObject, Dummy>() == 3u); // maintain last position to avoid breaking batches
        
        // animations
        static_assert(ctti::typeIndex<Animation, FramesSequence>() == 0u);
    }

}