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
class Queue;

class Swapchain {
public:
    Swapchain(const Device & device, const Surface & surface, const rhi::SwapchainCreateInfo & info);
    ~Swapchain();

    inline const VkSwapchainKHR & operator ()() const noexcept { return _swapchain; }
    inline explicit operator bool() const noexcept { return _swapchain != VK_NULL_HANDLE; }

    void resize(uint32_t w, uint32_t h, bool vsync);

    std::pair<uint32_t, bool> acquireNextImage(uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const noexcept;

    const VkSemaphore & getImageAvailableSemaphore(uint32_t frameIndex) const noexcept { return _imageAvailableSemaphores[frameIndex]; }
    void waitFenceAndReset(uint64_t timeout = UINT64_MAX) noexcept;

    uint32_t getImagesCount() const noexcept;
    VkImageView getImageView(uint8_t index) noexcept { return _imageViews[index]; }

    rhi::Format getFormat() const noexcept { return _format; }
    rhi::ColorSpace getColorSpace() const noexcept { return _colorSpace; }

    const Image & getImage(uint8_t index) const noexcept;
    Image & getImage(uint8_t index) noexcept;

    void present(const Queue & queue, uint8_t imageIndex) noexcept; // need to set presentInfo

private:
    void destroy(VkSwapchainKHR swapchain);
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    const Device & _device;
    const Surface & _surface;
    std::vector<Image> _images;
    std::vector<VkImageView> _imageViews;
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    VkFence _imageAvailableFence = VK_NULL_HANDLE;

    rhi::SwapchainCreateInfo _info;
    rhi::Format _format;
    rhi::ColorSpace _colorSpace;
};


}