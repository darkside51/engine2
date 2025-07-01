#include "barriers.h"

namespace engine::vulkan {

    constexpr uint8_t kBufferBarriersReserveCapacity = 8u;
    constexpr uint8_t kImageBarriersReserveCapacity = 8u;

    void PipelineBarriers::insertBufferBarrier(VkPipelineStageFlags srcStage, 
        VkPipelineStageFlags dstStage, 
        VkDependencyFlags flags, 
        VkBufferMemoryBarrier barrier) {

        auto & bufferBarriers = _barriers[{srcStage, dstStage, flags}].bufferBarriers;
        if (bufferBarriers.capacity() == 0u) {
            bufferBarriers.reserve(kBufferBarriersReserveCapacity);
        }

        bufferBarriers.emplace_back(barrier);
    }

    void PipelineBarriers::insertImageBarrier(VkPipelineStageFlags srcStage, 
        VkPipelineStageFlags dstStage, 
        VkDependencyFlags flags, 
        VkImageMemoryBarrier barrier) {

        auto & imageBarriers = _barriers[{srcStage, dstStage, flags}].imageBarriers;
        if (imageBarriers.capacity() == 0u) {
            imageBarriers.reserve(kImageBarriersReserveCapacity);
        }

        imageBarriers.emplace_back(barrier);
    }

    void PipelineBarriers::flush(VkCommandBuffer commandBuffer) {
        for (/*const*/ auto & [stage, barriers] : _barriers) {
            if (!barriers.bufferBarriers.empty() || !barriers.imageBarriers.empty()) {
                vkCmdPipelineBarrier(
                    commandBuffer, 
                    stage.srcStageMask, stage.dstStageMask, stage.dependencyFlags, 
                    0u, nullptr, // need to use mamory barriers?
                    barriers.bufferBarriers.size(), barriers.bufferBarriers.data(), 
                    barriers.imageBarriers.size(), barriers.imageBarriers.data()
                );

                // reset data from all reused vectors, for minimize dynamic allocations
                barriers.bufferBarriers.clear();
                barriers.imageBarriers.clear();
            }
        }

        //_barriers.clear(); // no clear all map, for minimize dynamic allocations
    }

    void PipelineBarriers::reset() {
        _barriers.clear();
    }
}