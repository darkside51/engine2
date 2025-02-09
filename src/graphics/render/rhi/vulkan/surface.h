#pragma once

#include "graphics/render/rhi/common/surface.h"

#include <vulkan/vulkan.h>

namespace engine::vulkan {

    class Instance;

    class Surface {
    public:
        Surface(const Instance & instance, const rhi::SurfaceCreateInfo & info);
        VkSurfaceKHR getVkSurface() const noexcept { return _surface; }

        inline VkSurfaceKHR operator ()() const noexcept { return _surface; }
        inline explicit operator bool() const noexcept { return _surface != VK_NULL_HANDLE; }

    private:
        rhi::SurfaceType _type;
        VkSurfaceKHR _surface = VK_NULL_HANDLE;
    };

}