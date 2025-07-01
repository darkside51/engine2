#pragma once

#include "common/hash.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace engine::vulkan {

class PipelineBarriers {
    struct Barriers {
        std::vector<VkBufferMemoryBarrier> bufferBarriers;
        std::vector<VkImageMemoryBarrier> imageBarriers;
    };

    struct BarrierStage {
        VkPipelineStageFlags srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkDependencyFlags dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
        size_t hashValue = 0u;

        BarrierStage(VkPipelineStageFlags srcStageMask, 
            VkPipelineStageFlags dstStageMask, 
            VkDependencyFlags dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT) :
            srcStageMask(srcStageMask),
            dstStageMask(dstStageMask),
            dependencyFlags(dependencyFlags),
            hashValue(hash::hash_combine(srcStageMask, dstStageMask, dependencyFlags)) {}

        inline bool operator == (const BarrierStage & other) const noexcept = default;
            
        inline size_t hash() const noexcept {
            return hashValue;
        }
    };

    using BarriersMap = std::unordered_map<BarrierStage, Barriers, 
                                            hash::Hasher<BarrierStage>, std::equal_to<>>;

public:
    void insertBufferBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkDependencyFlags flags, VkBufferMemoryBarrier barrier);
    void insertImageBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkDependencyFlags flags, VkImageMemoryBarrier barrier);

    void flush(VkCommandBuffer commandBuffer);
    void reset();

private:
    BarriersMap _barriers;
    
};

}