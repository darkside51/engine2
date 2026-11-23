#include "command_pool.h"
#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "vk_types.h"

#include "log/log.h"

#include <cassert>

namespace engine::vulkan {

    CommandPool::CommandPool(const Device & device, uint8_t queueFlags, rhi::CommandPoolCreateFlags poolFlags) :
        _device(device), _flags(poolFlags) {
        const auto [familyIndex, _] = _device.gpu().getQueueFamily(queueFlags, 0u);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = familyIndex;
        poolInfo.flags = utils::convert2vk(_flags);

        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        if (const auto result = vkCreateCommandPool(_device(), &poolInfo, allocationCallbacks, &_pool); result != VK_SUCCESS) {
            LOG_E("vulkan: vkCreateCommandPool result: %d", result);
            _pool = VK_NULL_HANDLE;
        }
    }

    CommandPool::~CommandPool() {
        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        vkDestroyCommandPool(_device(), _pool, allocationCallbacks);
    }

    VkCommandBuffer CommandPool::allocCommandBuffer(rhi::CommandBufferLevel level) {
        if (std::to_underlying(_flags & rhi::CommandPoolCreateFlags::eReset_Command_Buffer) != 0u) {
            auto & buffers = _freeBuffers[static_cast<uint8_t>(level)];
            if (!buffers.empty()) {
                VkCommandBuffer commandBuffer = buffers.back();
                buffers.pop_back();
                return commandBuffer;
            }
        }

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = utils::convert2vk(level);
        allocInfo.commandPool = _pool;
        allocInfo.commandBufferCount = 1u;

        VkCommandBuffer commandBuffer;
        if (const auto result = vkAllocateCommandBuffers(_device(), &allocInfo, &commandBuffer); result != VK_SUCCESS) {
            LOG_E("vulkan: vkAllocateCommandBuffers result: %d", result);
            return VK_NULL_HANDLE;
        }
        
        return commandBuffer;
    }

    void CommandPool::deallocCommandBuffer(rhi::CommandBufferLevel level, VkCommandBuffer commandBuffer) {
        if (std::to_underlying(_flags & rhi::CommandPoolCreateFlags::eReset_Command_Buffer) != 0u) {
            const auto result = vkResetCommandBuffer(commandBuffer, 0);
            assert(result == VK_SUCCESS);
            _freeBuffers[static_cast<uint8_t>(level)].push_back(commandBuffer);
        }
    }
}