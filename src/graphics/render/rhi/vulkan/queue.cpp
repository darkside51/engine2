#include "queue.h"
#include "device.h"
#include "command_buffer.h"

namespace engine::vulkan {
    
    bool Queue::waitIdle() noexcept {
        return vkQueueWaitIdle(_queue) == VK_SUCCESS;
    }

    bool Queue::waitSubmitComplete(uint8_t frameIdx, std::span<VkFence> fences, uint64_t timeout) noexcept {
        const auto * fence = fences.empty() ? &_submitCompleteFences[frameIdx] : &fences[frameIdx];
        const auto result = vkWaitForFences(_device(), 1, fence, VK_TRUE, timeout); 
        if (result != VK_SUCCESS) {
            return false;
        }
        vkResetFences(_device(), 1, fence);
        return true;
    }

    VkSubmitInfo Queue::submitInfo(const CommandBuffer & commandBuffer, const SubmitSemaphores & semaphores) noexcept {
        return VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(semaphores.wait.semaphores.size()),
            .pWaitSemaphores = semaphores.wait.semaphores.data(),
            .pWaitDstStageMask = semaphores.wait.dstStages.data(),
            .commandBufferCount = 1u,
            .pCommandBuffers = commandBuffer.getVkBufferPtr(),
            .signalSemaphoreCount = static_cast<uint32_t>(semaphores.signalSemaphores.size()),
            .pSignalSemaphores = semaphores.signalSemaphores.data()
        };
    }

}