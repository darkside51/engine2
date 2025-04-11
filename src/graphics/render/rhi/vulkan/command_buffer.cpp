#include "command_buffer.h"
#include "command_pool.h"
#include "framebuffer.h"
#include "vk_types.h"

namespace engine::vulkan {

    VkCommandBufferInheritanceInfo CommandBuffer::CommandBufferInheritanceInfo::operator()() const noexcept {
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
        if (_state.setViewPort({{{static_cast<int32_t>(x), static_cast<int32_t>(y)}, 
            {static_cast<uint32_t>(w), static_cast<uint32_t>(h)}}, minDepth, maxDepth})) {
            const VkViewport viewport{x, y, w, h, minDepth, maxDepth};
            vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
        }
    }

    void CommandBuffer::setScissorRect(const rhi::Rect2D & area) noexcept {
        const VkRect2D scissor{utils::convert2vk(area.offset), utils::convert2vk(area.extent)};
        if (_state.setScissor(area)) {
            vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
        }
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

    void CommandBuffer::copyBuffer2Buffer(const Buffer & src, const Buffer & dst, std::span<VkBufferCopy> copyInfo) const noexcept {
        // vkCmdCopyBuffer2(); // ?
        vkCmdCopyBuffer(_commandBuffer, src(), dst(), copyInfo.size(), copyInfo.data());
    }

    void CommandBuffer::copyBufferToImage(const Buffer & src, const Image & dst, std::span<VkBufferImageCopy> copyInfo) const noexcept {
        // vkCmdCopyBufferToImage2; // ?

        const auto info = dst.getInfo();
        const auto & subresource = copyInfo[0u].imageSubresource;
        const auto baseLayoutIdx = subresource.baseArrayLayer * info.mipLevels + subresource.mipLevel;
        const auto baseLayout = dst.getSubStates()[baseLayoutIdx].layout;

        vkCmdCopyBufferToImage(_commandBuffer, src(), dst(), baseLayout, copyInfo.size(), copyInfo.data());
    }

    void CommandBuffer::bindPipeline(Pipeline const & pipeline) noexcept {
        if (_pipeline.pipeline != pipeline.pipeline) {
            vkCmdBindPipeline(_commandBuffer, pipeline.bindPoint, pipeline.pipeline);
            _pipeline = pipeline;
        }
    }

    void CommandBuffer::bindVertexBuffers(std::span<const VkBuffer> buffers, std::span<const VkDeviceSize> offsets, uint32_t firstBinding) noexcept {
        vkCmdBindVertexBuffers(_commandBuffer, firstBinding, buffers.size(), buffers.data(), offsets.data());
    }

    void CommandBuffer::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, rhi::IndexType type) noexcept {
        vkCmdBindIndexBuffer(_commandBuffer, buffer, offset, utils::convert2vk(type));
    }

    void CommandBuffer::bindDescriptorSets(std::span<const VkDescriptorSet> sets, std::span<const uint32_t> dynamicOffsets, uint32_t firstSet) noexcept {
        vkCmdBindDescriptorSets(
            _commandBuffer, 
            _pipeline.bindPoint,
            _pipeline.layout,
            firstSet, 
            sets.size(), sets.data(),
            dynamicOffsets.size(), dynamicOffsets.data()
        );
    }

    void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const noexcept {
        vkCmdDraw(_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

}