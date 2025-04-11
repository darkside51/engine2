#pragma once

#include <vulkan/vulkan.h>

namespace engine::vulkan {

    class Pipeline {
    public:


    // private:
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
    };

}
