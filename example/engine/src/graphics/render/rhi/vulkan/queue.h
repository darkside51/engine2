#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <vector>

namespace engine::vulkan {

    class Device;
    class CommandBuffer;
    class Swapchain;
    struct SubmitSyncInfo;

    class Queue final : public rhi::runtime::Queue {
    public:
        Queue(const Device & device, rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount);
        ~Queue();
    
        inline VkQueue operator ()() const noexcept { return _queue; }
        inline explicit operator bool() const noexcept { return _queue != VK_NULL_HANDLE; }

        bool waitIdle() noexcept override;
        
        bool waitSubmitComplete(uint8_t frameIdx, uint64_t timeout = UINT64_MAX, VkFence f = VK_NULL_HANDLE) noexcept;
        VkSubmitInfo submitInfo(stl::span<const VkCommandBuffer> commandBuffers, const SubmitSyncInfo & syncInfo) noexcept;
        VkResult submit(stl::span<const VkSubmitInfo> submitInfos, uint8_t frameIdx, VkFence f = VK_NULL_HANDLE) noexcept;

        VkResult present(stl::span<const VkSwapchainKHR> swapchains, stl::span<const uint32_t> imageIdxs, stl::span<const VkSemaphore> waitSemaphores) noexcept;
        
        VkResult flushCommandBuffers(stl::span<const VkCommandBuffer> commandBuffers, VkFence extenalFence = VK_NULL_HANDLE) noexcept;

        // overrides
        bool waitSubmit(uint8_t frameIdx, uint64_t timeout = UINT64_MAX, rhi::runtime::Fence f = nullptr) noexcept override;
        bool submit(uint8_t frameIdx, stl::span<const rhi::runtime::Queue::SubmitInfo> submitInfos, rhi::runtime::Fence f = nullptr) noexcept override;
        bool present(const rhi::runtime::Swapchain * swapchains, uint32_t imageIdx, 
            stl::span<const rhi::runtime::Semaphore> waitSemaphores) noexcept override;

    private:
        stl::vector<VkFence> _submitCompleteFences;
        VkQueue _queue = VK_NULL_HANDLE;
        const Device & _device;
    };

}