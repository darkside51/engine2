#pragma once

#include "math.h"
#include "earcut.hpp"

namespace mapbox::util {

    template <typename T>
    struct nth<0u, engine::vec2<T>> {
        static auto get(engine::vec2<T> v) noexcept { return v.x; }
    };

    template <typename T>
    struct nth<1u, engine::vec2<T>> {
        static auto get(engine::vec2<T> v) noexcept { return v.y; }
    };
    
}

namespace engine {
    namespace earcut = mapbox::util;
}