#pragma once

#include "vk_types.h"

namespace engine::vulkan {
    struct Helper {
        template <typename...Args>
        static auto convert2api(Args&&...args) noexcept {
            return utils::convert2vk(std::forward<Args>(args)...);
        }

        template <typename...Args>
        static auto convert2rhi(Args&&...args) noexcept {
            return utils::convert2rhi(std::forward<Args>(args)...);
        }
    };
}