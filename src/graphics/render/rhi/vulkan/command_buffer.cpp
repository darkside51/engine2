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

    void CommandBuffer::setViewport(float x, float y, float w, float h, float minDepth, float maxDepth, uint32_t viewportIdx) noexcept {
        const VkViewport viewport{x, y, w, h, minDepth, maxDepth};
        vkCmdSetViewport(_commandBuffer, viewportIdx, 1, &viewport);
    }

    void CommandBuffer::setScissorRect(const rhi::Rect2D & area, uint32_t scessorIdx) noexcept {
        const VkRect2D scissor{utils::convert2vk(area.offset), utils::convert2vk(area.extent)};
        vkCmdSetScissor(_commandBuffer, scessorIdx, 1, &scissor);
    }

    rhi::Result CommandBuffer::begin(rhi::CommandBufferUsageFlags flags, const CommandBufferInheritanceInfo * inheritanceInfo) noexcept {
        //_state.reset();
        _pipeline = {};
        if (_level == rhi::CommandBufferLevel::ePrimary || inheritanceInfo == nullptr) {
            const VkCommandBufferBeginInfo beginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = utils::convert2vk(flags),
                .pInheritanceInfo = nullptr
            };
    
            vkBeginCommandBuffer(_commandBuffer, &beginInfo);
            return rhi::Result::eSuccess;
        } else {
            const auto info = inheritanceInfo->operator()();
            const VkCommandBufferBeginInfo beginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = utils::convert2vk(flags),
                .pInheritanceInfo = &info
            };
    
            vkBeginCommandBuffer(_commandBuffer, &beginInfo);
            return rhi::Result::eSuccess;
        }
    }

    void CommandBuffer::beginRenderPass(VkRenderPass renderPass, const FrameBuffer & framebuffer, 
        stl::span<VkClearValue> clearValues, const rhi::Rect2D & area/*, rhi::SubpassContents contents*/) const noexcept {
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

    void CommandBuffer::updateBuffer(const Buffer & dst, uint32_t dstOffset, uint32_t dataSize, const void* pData) const noexcept {
        vkCmdUpdateBuffer(_commandBuffer, dst(), dstOffset, dataSize, pData);
    }

    void CommandBuffer::copyBuffer2Buffer(const Buffer & src, const Buffer & dst, stl::span<VkBufferCopy> copyInfo) const noexcept {
        // vkCmdCopyBuffer2(); // ?
        vkCmdCopyBuffer(_commandBuffer, src(), dst(), copyInfo.size(), copyInfo.data());
    }

    void CommandBuffer::copyBuffer2Image(const Buffer & src, const Image & dst, stl::span<VkBufferImageCopy> copyInfo) const noexcept {
        // vkCmdCopyBufferToImage2; // ?

        const auto & info = dst.getInfo();
        const auto & subresource = copyInfo[0u].imageSubresource;
        const auto baseLayoutIdx = subresource.baseArrayLayer * info.mipLevels + subresource.mipLevel;
        const auto baseLayout = dst.getSubStates()[baseLayoutIdx].layout;

        vkCmdCopyBufferToImage(_commandBuffer, src(), dst(), baseLayout, copyInfo.size(), copyInfo.data());
    }

    void CommandBuffer::copyImage2Buffer(const Image & src, const Buffer & dst, stl::span<VkBufferImageCopy> copyInfo) const noexcept {
        const auto & info = src.getInfo();
        const auto & subresource = copyInfo[0u].imageSubresource;
        const auto baseLayoutIdx = subresource.baseArrayLayer * info.mipLevels + subresource.mipLevel;
        const auto baseLayout = src.getSubStates()[baseLayoutIdx].layout;

        vkCmdCopyImageToBuffer(_commandBuffer, src(), baseLayout, dst(), copyInfo.size(), copyInfo.data());
    }

    void CommandBuffer::blitImage(const Image & src, const Image & dst, VkImageLayout srcImageLayout, VkImageLayout dstImageLayout, stl::span<VkImageBlit> info, VkFilter filter) const noexcept {
        vkCmdBlitImage(_commandBuffer, src(), srcImageLayout, dst(), dstImageLayout, info.size(), info.data(), filter);
    }

    void CommandBuffer::copyImage2Image(const Image & src, const Image & dst, stl::span<VkImageCopy> copyInfo) const noexcept {
        const auto & srcInfo = src.getInfo();
        const auto & srcSubresource = copyInfo[0u].srcSubresource;
        const auto srcBaseLayoutIdx = srcSubresource.baseArrayLayer * srcInfo.mipLevels + srcSubresource.mipLevel;
        const auto srcBaseLayout = src.getSubStates()[srcBaseLayoutIdx].layout;

        const auto & dstInfo = dst.getInfo();
        const auto & dstSubresource = copyInfo[0u].dstSubresource;
        const auto dstBaseLayoutIdx = dstSubresource.baseArrayLayer * dstInfo.mipLevels + dstSubresource.mipLevel;
        const auto dstBaseLayout = dst.getSubStates()[dstBaseLayoutIdx].layout;

        vkCmdCopyImage(_commandBuffer, src(), srcBaseLayout, dst(), dstBaseLayout, copyInfo.size(), copyInfo.data());
    }

    void CommandBuffer::bindPipeline(Pipeline const & pipeline) noexcept {
        if (_pipeline.pipeline != pipeline.pipeline) {
            const auto vkBindPoint = utils::convert2vk(pipeline.bindPoint);
            vkCmdBindPipeline(_commandBuffer, vkBindPoint, pipeline.pipeline);
            _pipeline = { pipeline.pipeline, pipeline.layout, vkBindPoint };
        }
    }

    void CommandBuffer::bindVertexBuffer(uint32_t binding, const VkBuffer buffer, VkDeviceSize offset) noexcept {
        vkCmdBindVertexBuffers(_commandBuffer, binding, 1u, &buffer, &offset);
    }

    void CommandBuffer::bindVertexBuffers(stl::span<const VkBuffer> buffers, stl::span<const VkDeviceSize> offsets, uint32_t firstBinding) noexcept {
        vkCmdBindVertexBuffers(_commandBuffer, firstBinding, buffers.size(), buffers.data(), offsets.data());
    }

    void CommandBuffer::bindIndexBuffer(VkBuffer buffer, uint32_t offset, rhi::IndexType type) noexcept {
        vkCmdBindIndexBuffer(_commandBuffer, buffer, static_cast<VkDeviceSize>(offset), utils::convert2vk(type));
    }

    void CommandBuffer::bindDescriptorSets(stl::span<const VkDescriptorSet> sets, stl::span<const uint32_t> dynamicOffsets, uint32_t firstSet) noexcept {
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

    void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
        uint32_t firstInstance) const noexcept {
        vkCmdDrawIndexed(_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

}