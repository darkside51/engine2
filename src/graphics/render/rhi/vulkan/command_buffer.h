#pragma once

#include "graphics/render/rhi/common/command_buffer.h"
#include "graphics/render/rhi/common/types.h"
#include "buffer.h"
#include "image.h"
#include "pipeline.h"
#include "vk_types.h"

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

        // const rhi::CommandBufferState & getState() const noexcept { return _state; }

        rhi::Result begin(rhi::CommandBufferUsageFlags flags = rhi::CommandBufferUsageFlags::eOne_Time_Submit, 
            const CommandBufferInheritanceInfo * inheritanceInfo = nullptr) noexcept;

        inline rhi::Result end() const noexcept {
            vkEndCommandBuffer(_commandBuffer);
            return rhi::Result::eSuccess;
        }

        inline void reset(rhi::CommandBufferResetFlags flags) noexcept {
            vkResetCommandBuffer(_commandBuffer, utils::convert2vk(flags));
            _pipeline = {};
            // _state.reset();
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
        
        void setViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f, uint32_t viewportIdx = 0u) noexcept;
        void setScissorRect(const rhi::Rect2D & area, uint32_t scessorIdx = 0u) noexcept;

        void updateBuffer(const Buffer & dst, uint32_t dstOffset, uint32_t dataSize, const void* pData) const noexcept;
        void copyBuffer2Buffer(const Buffer & src, const Buffer & dst, std::span<VkBufferCopy> copyInfo) const noexcept;
        void copyBuffer2Image(const Buffer & src, const Image & dst, std::span<VkBufferImageCopy> copyInfo) const noexcept;
        void blitImage(const Image & src, const Image & dst, VkImageLayout srcImageLayout, VkImageLayout dstImageLayout, std::span<VkImageBlit> info, VkFilter filter) const noexcept;

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

        // bind commands
        void bindPipeline(Pipeline const & pipeline) noexcept;
        void bindVertexBuffer(uint32_t binding, const VkBuffer buffer, VkDeviceSize offset = 0u) noexcept;
        void bindVertexBuffers(std::span<const VkBuffer> buffers, std::span<const VkDeviceSize> offsets, uint32_t firstBinding = 0u) noexcept;
        void bindIndexBuffer(VkBuffer buffer, uint32_t offset = 0u, rhi::IndexType type = rhi::IndexType::eU32) noexcept;
        void bindDescriptorSets(std::span<const VkDescriptorSet> sets, std::span<const uint32_t> dynamicOffsets = {}, uint32_t firstSet = 0u) noexcept;

        // draw commands
        void draw(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) const noexcept;
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1u, uint32_t firstIndex = 0u, int32_t vertexOffset = 0u,
            uint32_t firstInstance = 0u) const noexcept;

    private:
        struct UsedPipeline {
            VkPipeline pipeline = VK_NULL_HANDLE;
            VkPipelineLayout layout = VK_NULL_HANDLE;
            VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
        }; 
        // rhi::CommandBufferState _state{};
        CommandPool & _pool;
        rhi::CommandBufferLevel _level{};
        VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
        UsedPipeline _pipeline{};
    };
}