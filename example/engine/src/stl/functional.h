#pragma once

#include "EASTL/functional.h"
// #include <functional>

namespace engine::stl {
    template <typename T>
	using equal_to = eastl::equal_to<T>;
    
    // template <typename T, typename U>
    // using equal_to = std::equal_to<T>;
}