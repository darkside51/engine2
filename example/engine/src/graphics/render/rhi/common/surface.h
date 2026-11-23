#pragma once

#include <cstdint>

namespace engine::rhi {

    enum class SurfaceType : uint8_t {
        eDisplay = 0u,
        eWindowSurface = 1u,
        eAndroid = 2u
    };

    struct SurfaceCreateInfo {
        SurfaceType type = SurfaceType::eWindowSurface;
        void * display = nullptr;
        void * window = nullptr;
    };
}