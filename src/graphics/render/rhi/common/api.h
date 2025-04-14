#pragma once

#include <string_view>

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
        using Buffer = T::Buffer;
        using Image = T::Image;
        using FrameBuffer = T::FrameBuffer;
        using CommandBuffer = T::CommandBuffer;
        using Queue = T::Queue;
        using GpuProgram = T::GpuProgram;
        using Pipeline = T::Pipeline;
        using GraphicsPipelineInfo = T::GraphicsPipelineInfo;
        using Helper = T::Helper;

        static consteval ApiType type() noexcept {
            return T::type();
        }

        static consteval std::string_view name() noexcept {
            return T::name();
        }
    };
}