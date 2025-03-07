#pragma once

#include "graphics/render/rhi/common/api.h"

#include <vulkan/vulkan.h>

namespace engine::vulkan {
    class Instance;
    class PhisicalDevice;
    class Device;
    class Surface;
    class Swapchain;
    class Image;
    class FrameBuffer;
    class RenderPassManager;
    
    class Api {
    public:
        using Instance = vulkan::Instance;
        using Device = vulkan::Device;
        using PhisicalDevice = vulkan::PhisicalDevice;
        using Surface = vulkan::Surface;
        using Swapchain = vulkan::Swapchain;
        using Image = vulkan::Image;
        using FrameBuffer = vulkan::FrameBuffer;
        using RenderPassManager = vulkan::RenderPassManager;

        static constexpr auto kSwapchainExtName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        static constexpr auto kDebugUtilExtName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        static constexpr auto kLayerValidationKHR = "VK_LAYER_KHRONOS_validation";
        
        static consteval rhi::ApiType type() noexcept {
            return rhi::ApiType::eVulkan;
        }
    };
}