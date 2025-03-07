#pragma once

#include <vulkan/vulkan.h>

#include <span>

namespace engine::vulkan {

    struct SubmitSemaphores {
        std::span<VkSemaphore> waitSemaphores;
        std::span<VkSemaphore> signalSemaphores;
    };

    class CommandBuffer {

    };
}