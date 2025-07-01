#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "barriers.h"
#include "command_buffer.h"

namespace engine::vulkan::runtime {

    class CommandList : public rhi::runtime::CommandList {
        struct PostBufferBarrierInfo {
            Buffer * buffer;
            size_t offset;
            size_t size;
        };

        struct PostImageBarrierInfo {
            Image * image;
        };

    public:
        CommandList(rhi::runtime::CommandPool * commandPool, 
            rhi::CommandBufferLevel level = rhi::CommandBufferLevel::ePrimary);
        ~CommandList() = default;

        void record(uint8_t frameIdx) noexcept override;

        const CommandBuffer & commandBuffer() const noexcept { return _commandbuffer; }

    private:
        void resetState() noexcept;

        CommandPool * _pool = nullptr;
        CommandBuffer _commandbuffer;

        PipelineBarriers _barriers;
        PipelineBarriers _postBarriers;

        std::vector<PostBufferBarrierInfo> _postBarriersBuffers;
        std::vector<PostImageBarrierInfo> _postBarriersImages;
    };
}