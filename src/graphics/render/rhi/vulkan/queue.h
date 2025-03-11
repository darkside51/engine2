#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <vector>

namespace engine::vulkan {

    class Device;
    class CommandBuffer;
    struct SubmitSemaphores;

    class Queue {
    public:
        Queue(const Device & device, uint8_t familyFlags, uint8_t queueIdx, uint8_t framesCount);
    
        inline VkQueue operator ()() const noexcept { return _queue; }
        inline explicit operator bool() const noexcept { return _queue != VK_NULL_HANDLE; }

        bool waitIdle() noexcept;
        bool waitSubmitComplete(uint8_t frameIdx, std::span<VkFence> fences, uint64_t timeout = UINT64_MAX) noexcept;
        VkSubmitInfo submitInfo(const CommandBuffer & commandBuffer, const SubmitSemaphores & semaphores) noexcept;
        void submit(uint8_t frameIdx, std::span<VkFence> fences) noexcept;

    private:
        std::vector<VkFence> _submitCompleteFences;
        VkQueue _queue = VK_NULL_HANDLE;
        Device & _device;
    };

}