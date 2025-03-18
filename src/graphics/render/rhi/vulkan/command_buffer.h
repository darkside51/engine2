#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <span>

namespace engine::vulkan {

    class CommandPool;
    class FrameBuffer;

    struct SubmitSyncInfo {
        struct Wait {
            std::span<const VkSemaphore> semaphores;
            std::span<const VkPipelineStageFlags> dstStages;
        };
        Wait wait;
        std::span<const VkSemaphore> signalSemaphores;
    };

    struct CommandBufferInheritanceInfo {
        VkRenderPass renderPass;
        uint32_t subpass;
        rhi::QueryControlFlags pipelineStatistics;
        rhi::QueryControlFlags queryFlags;
        const FrameBuffer & framebuffer;
        bool occlusionQueryEnable;

        VkCommandBufferInheritanceInfo operator()() const noexcept;
    };

    class CommandBuffer {
    public:
        CommandBuffer(CommandPool & pool, rhi::CommandBufferLevel level);
        ~CommandBuffer();
        
        inline const VkCommandBuffer & operator ()() const noexcept { return _commandBuffer; }
        inline explicit operator bool() const noexcept { return _commandBuffer != VK_NULL_HANDLE; }

        rhi::Result begin(const CommandBufferInheritanceInfo * inheritanceInfo = nullptr) const noexcept;

        inline rhi::Result end() const noexcept {
            vkEndCommandBuffer(_commandBuffer);
            return rhi::Result::eSuccess;
        }

        void beginRenderPass(VkRenderPass renderPass, const FrameBuffer & framebuffer, 
            std::span<VkClearValue> clearValues, const rhi::Rect2D & area/*, 
            rhi::SubpassContents contents = rhi::SubpassContents::ePrimaryBufferInline*/) const noexcept;

        inline void endRenderPass() const noexcept {
            vkCmdEndRenderPass(_commandBuffer);
        }

        void nextSubpass(rhi::SubpassContents contents) const noexcept;

        void executeCommands(std::span<const VkCommandBuffer> secondaryCommandBuffers) const noexcept {
            vkCmdExecuteCommands(_commandBuffer, secondaryCommandBuffers.size(), secondaryCommandBuffers.data());
        }

        void setViewport(float x, float y, float w, float h, float minDepth, float maxDepth) noexcept;
        void setScissorRect(const rhi::Rect2D & area) noexcept;

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