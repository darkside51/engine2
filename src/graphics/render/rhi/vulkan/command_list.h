#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "command_buffer.h"

#include <vector>

namespace engine::vulkan::runtime {

    class CommandList : public rhi::runtime::CommandList {
    public:
        CommandList(rhi::runtime::CommandPool * commandPool, 
            rhi::CommandBufferLevel level = rhi::CommandBufferLevel::ePrimary);
        ~CommandList() = default;

        void record(uint8_t frameIdx) noexcept override;

        const CommandBuffer & commandBuffer() const noexcept { return _commandbuffer; }

    private:
        void resetState() noexcept;

        void addBufferBarrier(
            VkPipelineStageFlags srcStageMask, 
            VkPipelineStageFlags dstStageMask, 
            VkDependencyFlags dependencyFlags, 
            VkBufferMemoryBarrier barrier
        );

        void flushBarriers() noexcept;
        CommandPool * _pool = nullptr;
        CommandBuffer _commandbuffer;

        std::vector<VkBufferMemoryBarrier> _bufferBarriers;
        VkPipelineStageFlags _srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags _dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkDependencyFlags _dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
    };
}