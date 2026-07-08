#pragma once

#include "EASTL/utility.h"
// #include <utility>

namespace engine::stl {
    template <typename T, typename U>
    using pair = eastl::pair<T, U>;
    
    // template <typename T, typename U>
    // using pair = std::pair<T, U>;
}