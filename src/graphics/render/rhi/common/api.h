#pragma once

namespace engine::rhi {

    enum class ApiType : uint8_t {
        eVulkan = 0u,
        eOpenGL = 1u,
        eOpenGLES = 2u,
        eDirectX11 = 3u,
        eDirectX12 = 4u,
        eMetal = 5u,
        eMantle = 6u,
    };

    template<typename T>
    class Api {
    public:
        using Impl = T;
        using Instance = T::Instance;
        using Device = T::Device;
        using PhisicalDevice = T::PhisicalDevice;
        using Surface = T::Surface;
        using Swapchain = T::Swapchain;
        using Image = T::Image;
        using FrameBuffer = T::FrameBuffer;
        using RenderPassManager = T::RenderPassManager;

        static consteval ApiType type() noexcept {
            return T::type();
        }
    };
}