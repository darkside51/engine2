#pragma once

#include "graphics/render/rhi/common/swapchain.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace engine::vulkan {

class Device;
class Surface;
class Image;

class Swapchain {
public:
    Swapchain(const Device & device, const Surface & surface, const rhi::SwapchainCreateInfo & info);
    ~Swapchain();

    inline VkSwapchainKHR operator ()() const noexcept { return _swapchain; }
    inline explicit operator bool() const noexcept { return _swapchain != VK_NULL_HANDLE; }

    void resize(uint32_t w, uint32_t h, bool vsync);

    std::pair<uint32_t, bool> acquireNextImage(uint32_t frameIndex) const noexcept;

    VkSemaphore getImageAvailableSemaphore(uint32_t frameIndex) const noexcept { return _imageAvailableSemaphores[frameIndex]; }
    uint32_t getImagesCount() const noexcept;
    VkImageView getImageView(uint8_t index) noexcept { return _imageViews[index]; }

private:
    void destroy(VkSwapchainKHR swapchain);
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    const Device & _device;
    const Surface & _surface;
    std::vector<Image> _images;
    std::vector<VkImageView> _imageViews;
    std::vector<VkSemaphore> _imageAvailableSemaphores;

    rhi::SwapchainCreateInfo _info;
};


}