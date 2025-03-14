#include "queue.h"
#include "device.h"
#include "command_buffer.h"

#include <array>
#include <cassert>

namespace engine::vulkan {
    
    Queue::Queue(const Device & device, uint8_t familyFlags, uint8_t queueIdx, uint8_t framesCount) :
        _submitCompleteFences(framesCount), 
        _device(device) {
            const auto * queueFamily = device.findQueueFamily(familyFlags);
            if (queueFamily && queueIdx < queueFamily->queues.size()) {
                _queue = queueFamily->queues[queueIdx];

                for (size_t i = 0; i < framesCount; ++i) {
                    _submitCompleteFences[i] = device.createFence(engine::rhi::FenceCreateFlags::eSignaled);
                }
            }
    }

    Queue::~Queue() {
        for (auto & f : _submitCompleteFences) {
            _device.destroyFence(f);
        }
    }

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

    VkSubmitInfo Queue::submitInfo(std::span<const VkCommandBuffer> commandBuffers, const SubmitSyncInfo & syncInfo) noexcept {
        return VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(syncInfo.wait.semaphores.size()),
            .pWaitSemaphores = syncInfo.wait.semaphores.data(),
            .pWaitDstStageMask = syncInfo.wait.dstStages.data(),
            .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
            .pCommandBuffers = commandBuffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(syncInfo.signalSemaphores.size()),
            .pSignalSemaphores = syncInfo.signalSemaphores.data()
        };
    }

    VkResult Queue::submit(std::span<const VkSubmitInfo> submitInfos, uint8_t frameIdx, std::span<VkFence> fences) noexcept {
        const auto & fence = fences.empty() ? _submitCompleteFences[frameIdx] : fences[frameIdx];
        return vkQueueSubmit(_queue, submitInfos.size(), submitInfos.data(), fence);
    }

}