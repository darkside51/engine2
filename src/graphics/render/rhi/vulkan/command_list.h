#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "barriers.h"
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

        CommandPool * _pool = nullptr;
        CommandBuffer _commandbuffer;

        PipelineBarriers _barriers;
    };
}