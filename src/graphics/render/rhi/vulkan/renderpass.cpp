#include "renderpass.h"
#include "device.h"
#include "instance.h"
#include "physical_device.h"

namespace engine::vulkan {

    RenderPassManager::~RenderPassManager() {
        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        for (auto & [key, pass] : _renderPassCollection) {
            vkDestroyRenderPass(_device(), pass, allocationCallbacks);
        }
        _renderPassCollection.clear();
    }

    VkRenderPass RenderPassManager::getRenderPass(const rhi::RenderPassDescription & description) {
    //     VkResult vkCreateRenderPass(
    // VkDevice                                    device,
    // const VkRenderPassCreateInfo*               pCreateInfo,
    // const VkAllocationCallbacks*                pAllocator,
    // VkRenderPass*                               pRenderPass);
        return VK_NULL_HANDLE;
    }

}