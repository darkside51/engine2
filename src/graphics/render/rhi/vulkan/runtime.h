#pragma once

#include "api.h"
#include "instance.h"
#include "graphics/render/rhi/common/runtime.h"

namespace engine::vulkan::runtime {

    class Rhi final : public rhi::runtime::Rhi {
    public:
        using Api = rhi::Api<vulkan::Api>;
        explicit Rhi(const rhi::InstanceCreateInfo & instanceCreateInfo) : _instance(instanceCreateInfo) {}
    
        rhi::runtime::Instance & getInstance() noexcept override { return _instance; }
    
        rhi::runtime::SurfacePtr createSurface(const rhi::SurfaceCreateInfo & info) const override;

        std::vector<rhi::runtime::PysicalDevicePtr> enumerateDevices(const rhi::PhysicalDeviceRequirements & info) const override;
        rhi::runtime::DevicePtr createDevice(rhi::runtime::PysicalDevicePtr & gpu, const rhi::DeviceCreateInfo & info) const override;
    
        rhi::runtime::SwapchainPtr createSwapchain(const rhi::runtime::DevicePtr & device, 
            const rhi::runtime::SurfacePtr & surface, const rhi::SwapchainCreateInfo & info) const override;
    
        rhi::runtime::ImagePtr createImage(const rhi::runtime::DevicePtr & device, const rhi::ImageInfo & info) const noexcept override;
    
        rhi::runtime::RenderPass getRenderPass(rhi::runtime::DevicePtr & device, 
            const rhi::RenderPassDescription & description) const noexcept override;
    
        rhi::runtime::FrameBufferPtr createFrameBuffer(
            const rhi::runtime::DevicePtr & device, 
            std::vector<rhi::FrameBufferAttachment> && attachemnts, 
            const rhi::runtime::RenderPass & renderPass) const noexcept override;
    
        rhi::runtime::QueuePtr getQueue(const rhi::runtime::DevicePtr & device, 
            rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) const override;
    
        rhi::runtime::Fence createFence(const rhi::runtime::DevicePtr & device, 
            rhi::FenceCreateFlags flags) const noexcept override;

        rhi::runtime::Semaphore createSemaphore(const rhi::runtime::DevicePtr & device) const noexcept override;
    
        void destroyFence(const rhi::runtime::DevicePtr & device, 
            const rhi::runtime::Fence & fence) const noexcept override;

        void destroySemaphore(const rhi::runtime::DevicePtr & device, 
            const rhi::runtime::Semaphore & semaphore) const noexcept override;
    
        rhi::runtime::ShaderModulesManagerPtr createShaderModulesManager() const override;
    
        rhi::runtime::GpuProgramPtr createProgram(
            rhi::runtime::DevicePtr & device, 
            std::span<const rhi::runtime::ShaderModuleView> moduleViews
        ) const override;
    
        std::unique_ptr<engine::rhi::runtime::Pipeline> createComputePipeline(
            rhi::runtime::DevicePtr & device,
            const rhi::runtime::GpuProgramPtr & program) const noexcept override;
    
        std::unique_ptr<engine::rhi::runtime::Pipeline> createGraphicsPipeline(
            rhi::runtime::DevicePtr & device,
            rhi::runtime::RenderPass renderPass,
            const rhi::runtime::GpuProgramPtr & program,
            const rhi::GraphicsPipelineStatesInfo & info) const noexcept override;
    
    private:
        Api::Instance _instance;
    };

}