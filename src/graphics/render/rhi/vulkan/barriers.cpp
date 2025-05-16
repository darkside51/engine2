#include "barriers.h"

namespace engine::vulkan {

    constexpr uint8_t kBufferBarriersReserveCapacity = 8u;

    void PipelineBarriers::insertBufferBarrier(VkPipelineStageFlags srcStage, 
        VkPipelineStageFlags dstStage, 
        VkDependencyFlags flags, 
        VkBufferMemoryBarrier barrier) {

        auto & bufferBarriers = _barriers[{srcStage, dstStage, flags}].bufferBarriers;
        if (bufferBarriers.capacity() == 0u) {
            bufferBarriers.reserve(kBufferBarriersReserveCapacity);
        }

        _barriers[{srcStage, dstStage, flags}].bufferBarriers.emplace_back(barrier);
    }

    void PipelineBarriers::flush(VkCommandBuffer commandBuffer) {
        for (const auto & [stage, barriers] : _barriers) {
            vkCmdPipelineBarrier(
                commandBuffer, 
                stage.srcStageMask, stage.dstStageMask, stage.dependencyFlags, 
                0u, nullptr, 
                barriers.bufferBarriers.size(), barriers.bufferBarriers.data(), 
                0u, nullptr
            );
        }

        _barriers.clear();
    }

    void PipelineBarriers::reset() {
        _barriers.clear();
    }
}