#include "command_buffer.h"
#include "command_pool.h"
#include "framebuffer.h"
#include "vk_types.h"

namespace engine::vulkan {

    VkCommandBufferInheritanceInfo CommandBufferInheritanceInfo::operator()() const noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = nullptr,   // VUID-VkCommandBufferInheritanceInfo-pNext-pNext
                                // Each pNext member of any structure (including this one) in the pNext chain must be either NULL or a pointer to a valid instance of 
                                // VkAttachmentSampleCountInfoAMD, VkCommandBufferInheritanceConditionalRenderingInfoEXT, 
                                // VkCommandBufferInheritanceRenderPassTransformInfoQCOM, VkCommandBufferInheritanceRenderingInfo, 
                                // VkCommandBufferInheritanceViewportScissorInfoNV, VkExternalFormatANDROID, 
                                // VkMultiviewPerViewAttributesInfoNVX, VkRenderingAttachmentLocationInfo, 
                                // or VkRenderingInputAttachmentIndexInfo
            .renderPass = renderPass,
            .subpass = subpass,
            .framebuffer = framebuffer(),
            .occlusionQueryEnable = occlusionQueryEnable,
            .queryFlags = utils::convert2vk(queryFlags),
            .pipelineStatistics = utils::convert2vk(pipelineStatistics)
        };
    }

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

    rhi::Result CommandBuffer::begin(const CommandBufferInheritanceInfo * inheritanceInfo) const noexcept {
        if (_level == rhi::CommandBufferLevel::ePrimary || inheritanceInfo == nullptr) {
            const VkCommandBufferBeginInfo beginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0, // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - it here
                .pInheritanceInfo = nullptr
            };
    
            vkBeginCommandBuffer(_commandBuffer, &beginInfo);
            return rhi::Result::eSuccess;
        } else {
            const auto info = inheritanceInfo->operator()();
            const VkCommandBufferBeginInfo beginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0, // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - it here
                .pInheritanceInfo = &info
            };
    
            vkBeginCommandBuffer(_commandBuffer, &beginInfo);
            return rhi::Result::eSuccess;
        }
    }

    void CommandBuffer::beginRenderPass(VkRenderPass renderPass, const FrameBuffer & framebuffer, 
        std::span<VkClearValue> clearValues, const rhi::Rect2D & area/*, rhi::SubpassContents contents*/) const noexcept {
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

    void CommandBuffer::nextSubpass(rhi::SubpassContents contents) const noexcept {
        // vkCmdNextSubpass2();  // want it?
        vkCmdNextSubpass(_commandBuffer, utils::convert2vk(contents));
    }

}