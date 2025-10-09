#pragma once

#include "common/type_id.h"
#include "object.h"

namespace engine::scene {

    static consteval void registerTypes() noexcept {
        static_assert(ctti::typeIndex<Object, EmptyObject>() == 0u);
        
    }

    // static consteval 

}