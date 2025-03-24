#pragma once

#include "types.h"

#include <cstdint>

namespace engine::rhi {

    struct CommandBufferState {
        Viewport viewport{};
        Rect2D scissor{};

        bool setViewPort(const Viewport & v) noexcept {
            if (viewport == v) {
                return false;
            }
            viewport = v;
            return true;
        }

        bool setScissor(const Rect2D & s) noexcept {
            if (scissor == s) {
                return false;
            }
            scissor = s;
            return true;
        }
    };

}
