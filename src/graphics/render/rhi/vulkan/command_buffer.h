#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <span>

namespace engine::vulkan {

    class CommandPool;

    struct SubmitSyncInfo {
        struct Wait {
            std::span<const VkSemaphore> semaphores;
            std::span<const VkPipelineStageFlags> dstStages;
        };
        Wait wait;
        std::span<const VkSemaphore> signalSemaphores;
    };

    class CommandBuffer {
    public:
        CommandBuffer(CommandPool & pool, rhi::CommandBufferLevel level);
        ~CommandBuffer();
        
        inline const VkCommandBuffer & operator ()() const noexcept { return _commandBuffer; }
        inline explicit operator bool() const noexcept { return _commandBuffer != VK_NULL_HANDLE; }

        rhi::Result begin() noexcept;
        rhi::Result end() noexcept;

        void beginRenderPass() noexcept;
        void endRenderPass() noexcept;

        void setViewport() noexcept;
        void setScissorRect() noexcept;

        static SubmitSyncInfo createSubmitSyncInfo(
            std::span<const VkPipelineStageFlags> waitStages,
            std::span<const VkSemaphore> waitSemaphores,
            std::span<const VkSemaphore> signalSemaphores
        ) noexcept {
            return {
                .wait {
                    .semaphores = waitSemaphores,
                    .dstStages = waitStages
                },
                .signalSemaphores = signalSemaphores
            };
        }

    private:
        CommandPool & _pool;
        rhi::CommandBufferLevel _level{};
        VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    };
}