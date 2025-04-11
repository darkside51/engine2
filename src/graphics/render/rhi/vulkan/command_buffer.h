#pragma once

#include "graphics/render/rhi/common/command_buffer.h"
#include "graphics/render/rhi/common/types.h"
#include "buffer.h"
#include "image.h"
#include "pipeline.h"

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

    class CommandBuffer {
    public:
        struct CommandBufferInheritanceInfo {
            VkRenderPass renderPass;
            uint32_t subpass;
            rhi::QueryControlFlags pipelineStatistics;
            rhi::QueryControlFlags queryFlags;
            const FrameBuffer & framebuffer;
            bool occlusionQueryEnable;

            VkCommandBufferInheritanceInfo operator()() const noexcept;
        };


        CommandBuffer(CommandPool & pool, rhi::CommandBufferLevel level);
        ~CommandBuffer();
        
        inline const VkCommandBuffer & operator ()() const noexcept { return _commandBuffer; }
        inline explicit operator bool() const noexcept { return _commandBuffer != VK_NULL_HANDLE; }

        const rhi::CommandBufferState & getState() const noexcept { return _state; }

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

        void bindPipeline(Pipeline const & pipeline) noexcept;
        void bindVertexBuffers(std::span<const VkBuffer> buffers, std::span<const VkDeviceSize> offsets, uint32_t firstBinding = 0u) noexcept;
        void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, rhi::IndexType type = rhi::IndexType::eU32) noexcept;
        void bindDescriptorSets(std::span<const VkDescriptorSet> sets, std::span<const uint32_t> dynamicOffsets, uint32_t firstSet = 0u) noexcept;

        void draw(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) const noexcept;

        void setViewport(float x, float y, float w, float h, float minDepth, float maxDepth) noexcept;
        void setScissorRect(const rhi::Rect2D & area) noexcept;

        void copyBuffer2Buffer(const Buffer & src, const Buffer & dst, std::span<VkBufferCopy> copyInfo) const noexcept;
        void copyBufferToImage(const Buffer & src, const Image & dst, std::span<VkBufferImageCopy> copyInfo) const noexcept;

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
        rhi::CommandBufferState _state{};
        CommandPool & _pool;
        rhi::CommandBufferLevel _level{};
        VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
        Pipeline _pipeline{};
    };
}