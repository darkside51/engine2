#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace engine::vulkan {

    class Device;

    class Queue {
    public:
        Queue(const Device & device, uint8_t familyFlags, uint8_t queueIdx);
    
        inline VkQueue operator ()() const noexcept { return _queue; }
        inline explicit operator bool() const noexcept { return _queue != VK_NULL_HANDLE; }

        bool waitIdle() noexcept;
        bool waitSubmitComplete(uint8_t frameIdx) noexcept;
        void submit(uint8_t frameIdx) noexcept;

    private:
        VkQueue _queue = VK_NULL_HANDLE;
    };

}