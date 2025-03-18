#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <vector>

namespace engine::vulkan {

    class Device;
    class CommandBuffer;
    class Swapchain;
    struct SubmitSyncInfo;

    class Queue {
    public:
        Queue(const Device & device, uint8_t familyFlags, uint8_t queueIdx, uint8_t framesCount);
        ~Queue();
    
        inline VkQueue operator ()() const noexcept { return _queue; }
        inline explicit operator bool() const noexcept { return _queue != VK_NULL_HANDLE; }

        bool waitIdle() noexcept;
        
        bool waitSubmitComplete(uint8_t frameIdx, uint64_t timeout = UINT64_MAX, VkFence f = VK_NULL_HANDLE) noexcept;
        VkSubmitInfo submitInfo(std::span<const VkCommandBuffer> commandBuffers, const SubmitSyncInfo & syncInfo) noexcept;
        VkResult submit(std::span<const VkSubmitInfo> submitInfos, uint8_t frameIdx, VkFence f = VK_NULL_HANDLE) noexcept;

        VkResult present(std::span<const VkSwapchainKHR> swapchains, std::span<const uint32_t> imageIdxs, std::span<const VkSemaphore> waitSemaphores) noexcept;
        
        VkResult flushCommandBuffers(std::span<const VkCommandBuffer> commandBuffers, VkFence extenalFence = VK_NULL_HANDLE) noexcept;

    private:
        std::vector<VkFence> _submitCompleteFences;
        VkQueue _queue = VK_NULL_HANDLE;
        const Device & _device;
    };

}