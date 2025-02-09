#pragma once

#include "types.h"

#include <algorithm>
#include <cstdint>

namespace engine::rhi {

inline Extent2D getMipLevelSize(Extent2D size, uint8_t level) noexcept {
    return {std::max(size.width >> level, 1u), 
            std::max(size.height >> level, 1u)};
}

}