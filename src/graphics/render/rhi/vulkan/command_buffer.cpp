#include "command_buffer.h"
#include "command_pool.h"
#include "framebuffer.h"
#include "vk_types.h"

namespace engine::vulkan {

    CommandBuffer::CommandBuffer(CommandPool & pool, rhi::CommandBufferLevel level) : 
        _pool(pool), 
        _level(level), 
        _commandBuffer(pool.allocCommandBuffer(level)) {}

    CommandBuffer::~CommandBuffer() {
        _pool.deallocCommandBuffer(_level, _commandBuffer);
    }

    void CommandBuffer::setViewport(float x, float y, float w, float h, float minDepth, float maxDepth) noexcept {
        const VkViewport viewport{x, y, w, h, minDepth, maxDepth};
        vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
    }

    void CommandBuffer::setScissorRect(const rhi::Rect2D & area) noexcept {
        const VkRect2D scissor{utils::convert2vk(area.offset), utils::convert2vk(area.extent)};
        vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    }

    rhi::Result CommandBuffer::begin() noexcept {
        // typedef struct VkCommandBufferInheritanceInfo {
        //     VkStructureType                  sType;
        //     const void*                      pNext;
        //     VkRenderPass                     renderPass;
        //     uint32_t                         subpass;
        //     VkFramebuffer                    framebuffer;
        //     VkBool32                         occlusionQueryEnable;
        //     VkQueryControlFlags              queryFlags;
        //     VkQueryPipelineStatisticFlags    pipelineStatistics;
        // } VkCommandBufferInheritanceInfo;

        const VkCommandBufferBeginInfo beginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0, // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - it here
            .pInheritanceInfo = _level == rhi::CommandBufferLevel::ePrimary ? nullptr : nullptr // todo! need set info for secondary buffer
        };

		vkBeginCommandBuffer(_commandBuffer, &beginInfo);
        return rhi::Result::eSuccess;
    }

    void CommandBuffer::beginRenderPass(VkRenderPass renderPass, const FrameBuffer & framebuffer, 
        std::span<VkClearValue> clearValues, const rhi::Rect2D & area/*, rhi::SubpassContents contents*/) noexcept {
        const VkRenderPassBeginInfo renderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = renderPass,
            .framebuffer = framebuffer(),
            .renderArea = { utils::convert2vk(area.offset), utils::convert2vk(area.extent) },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data()
        };

        vkCmdBeginRenderPass(_commandBuffer, &renderPassBeginInfo, 
            (_level == rhi::CommandBufferLevel::ePrimary ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS));
    }

}