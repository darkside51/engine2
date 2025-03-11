#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <span>

namespace engine::vulkan {

    struct SubmitSemaphores {
        struct Wait {
            std::span<VkSemaphore> semaphores;
            std::span<VkPipelineStageFlags> dstStages;
        };
        Wait wait;
        std::span<VkSemaphore> signalSemaphores;
    };

    class CommandBuffer {
    public:
        inline VkCommandBuffer operator ()() const noexcept { return _commandBuffer; }
        inline explicit operator bool() const noexcept { return _commandBuffer != VK_NULL_HANDLE; }
        inline const VkCommandBuffer* getVkBufferPtr() const noexcept { return &_commandBuffer; }

    private:
        VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    };
}