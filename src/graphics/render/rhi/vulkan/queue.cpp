#include "queue.h"
#include "device.h"
#include "swapchain.h"
#include "command_buffer.h"
#include "command_list.h"
#include "runtime.h"

#include "log/log.h"

#include <array>
#include <cassert>
#include <utility>

namespace engine::vulkan {
    
    Queue::Queue(const Device & device, rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) :
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
        waitIdle();
        for (auto & f : _submitCompleteFences) {
            _device.destroyFence(f);
        }
    }

    bool Queue::waitIdle() noexcept {
        return vkQueueWaitIdle(_queue) == VK_SUCCESS;
    }

    bool Queue::waitSubmitComplete(uint8_t frameIdx, uint64_t timeout, VkFence f) noexcept {
        const auto * fence = (f == VK_NULL_HANDLE ? &_submitCompleteFences[frameIdx] : &f);
        const auto result = vkWaitForFences(_device(), 1u, fence, VK_TRUE, timeout); 
        if (result != VK_SUCCESS) { [[unlikely]]
            LOG_W("Queue vkWaitForFences result no success: %d", std::to_underlying(result));
            return false;
        }
        vkResetFences(_device(), 1, fence);
        return true;
    }

    VkSubmitInfo Queue::submitInfo(stl::span<const VkCommandBuffer> commandBuffers, const SubmitSyncInfo & syncInfo) noexcept {
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

    VkResult Queue::submit(stl::span<const VkSubmitInfo> submitInfos, uint8_t frameIdx, VkFence f) noexcept {
        const auto & fence = (f == VK_NULL_HANDLE ? _submitCompleteFences[frameIdx] : f);
        return vkQueueSubmit(_queue, submitInfos.size(), submitInfos.data(), fence);
    }

    VkResult Queue::flushCommandBuffers(stl::span<const VkCommandBuffer> commandBuffers, VkFence extenalFence) noexcept {
        auto fence = (extenalFence == VK_NULL_HANDLE ? _device.createFence(engine::rhi::FenceCreateFlags::eUnsignaled) : extenalFence);

        const stl::array submitInfos = {
            submitInfo(commandBuffers, {
                .wait {
                    .semaphores = {},
                    .dstStages = {}
            },
            .signalSemaphores = {}})
        };

        constexpr uint8_t kUndefinedFrame = 0xffu;

        auto result = submit(submitInfos, kUndefinedFrame, fence);
        waitSubmitComplete(kUndefinedFrame, UINT64_MAX, fence);

        if (extenalFence == VK_NULL_HANDLE) {
            _device.destroyFence(fence);
        }
        return result;
    }

    VkResult Queue::present(stl::span<const VkSwapchainKHR> swapchains, stl::span<const uint32_t> imageIdxs, stl::span<const VkSemaphore> waitSemaphores) noexcept {
        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		    .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = static_cast<uint32_t>(swapchains.size()),
		    .pSwapchains = swapchains.data(),
            .pImageIndices = imageIdxs.data(),
            .pResults = nullptr // todo!
        };

        return vkQueuePresentKHR(_queue, &presentInfo);
    }

    //// override
    bool Queue::waitSubmit(uint8_t frameIdx, uint64_t timeout, rhi::runtime::Fence f) noexcept {
        return waitSubmitComplete(frameIdx, timeout, f ? static_cast<VkFence>(f): VK_NULL_HANDLE);
    }

    bool Queue::submit(uint8_t frameIdx, stl::span<const rhi::runtime::Queue::SubmitInfo> submitInfos, rhi::runtime::Fence f) noexcept {
        const auto & fence = (f == nullptr ? _submitCompleteFences[frameIdx] : static_cast<VkFence>(f));

        struct SubmitTmpData {
            dynamic_array<VkCommandBuffer, 4u> commandBuffers;
            dynamic_array<VkPipelineStageFlags, 4u> pipelineStageFlags;
        };

        dynamic_array<SubmitTmpData, 4u> submitDatas;
        submitDatas.resize(submitInfos.size());

        dynamic_array<VkSubmitInfo, 4u> vkSubmitInfos;
        vkSubmitInfos.resize(submitInfos.size());

        size_t n = 0u;
        for (const auto & info : submitInfos) {
            SubmitTmpData & tmp = submitDatas.view()[n];

            tmp.commandBuffers.resize(info.lists.size());
            auto cmdBuffersView = tmp.commandBuffers.view();
            size_t i = 0u;
            for (const auto * list : info.lists) {
                const auto * vkList = static_cast<const runtime::CommandList*>(list);
                cmdBuffersView[i++] = vkList->commandBuffer()();
            }

            tmp.pipelineStageFlags.resize(info.w8Semaphores.size());
            auto pipelineStageFlagsView = tmp.pipelineStageFlags.view();

            i = 0u;
            for (const auto & stageFlags : info.w8DstStages) {
                pipelineStageFlagsView[i++] = utils::convert2vk(stageFlags);
            }

            vkSubmitInfos.view()[n] = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = static_cast<uint32_t>(info.w8Semaphores.size()),
                .pWaitSemaphores = reinterpret_cast<const VkSemaphore*>(info.w8Semaphores.data()),
                .pWaitDstStageMask = pipelineStageFlagsView.data(),
                .commandBufferCount = static_cast<uint32_t>(info.lists.size()),
                .pCommandBuffers = cmdBuffersView.data(),
                .signalSemaphoreCount = static_cast<uint32_t>(info.signalSemaphores.size()),
                .pSignalSemaphores = reinterpret_cast<const VkSemaphore*>(info.signalSemaphores.data())
            };

            ++n;
        }

        return vkQueueSubmit(_queue, submitInfos.size(), vkSubmitInfos.view().data(), fence) == VK_SUCCESS;
    }

    bool Queue::present(const rhi::runtime::Swapchain * swapchain, uint32_t imageIdx, 
        stl::span<const rhi::runtime::Semaphore> waitSemaphores) noexcept {

        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = reinterpret_cast<const VkSemaphore*>(waitSemaphores.data()),
            .swapchainCount = 1,
            .pSwapchains = &(*static_cast<const Swapchain *>(swapchain))(),
            .pImageIndices = &imageIdx,
            .pResults = nullptr // todo!
        };

        return vkQueuePresentKHR(_queue, &presentInfo) == VK_SUCCESS;
    }

}