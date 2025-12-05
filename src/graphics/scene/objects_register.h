#pragma once

#include "common/type_id.h"
#include "object.h"
#include "sprite/sprite.h"

namespace engine::scene {

    static consteval void registerTypes() noexcept {
        static_assert(ctti::typeIndex<Object, Dummy>() == 0u);
        static_assert(ctti::typeIndex<Object, Sprite>() == 1u);
    }

    // static consteval 

}