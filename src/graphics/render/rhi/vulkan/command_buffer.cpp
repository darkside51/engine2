#include "command_buffer.h"
#include "command_pool.h"

namespace engine::vulkan {

    CommandBuffer::CommandBuffer(CommandPool & pool, rhi::CommandBufferLevel level) : 
        _pool(pool), 
        _level(level), 
        _commandBuffer(pool.allocCommandBuffer(level)) {}

        CommandBuffer::~CommandBuffer() {
            _pool.deallocCommandBuffer(_level, _commandBuffer);
        }

}