#include "runtime.h"

#include "surface.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "framebuffer.h"
#include "queue.h"
#include "gpu_program.h"
#include "pipeline.h"
#include "buffer.h"

#include "graphics/render/rhi/common/shader_module.h"

namespace engine::vulkan::runtime {

    rhi::runtime::SurfacePtr Rhi::createSurface(const rhi::SurfaceCreateInfo & info) const {
        return std::make_unique<Api::Surface>(_instance, info);
    }

    std::vector<rhi::runtime::PysicalDevicePtr> Rhi::enumerateDevices(const rhi::PhysicalDeviceRequirements & info) const {
        auto apiPhysicalDevices = Api::PhysicalDevice::getSuitableDevices(_instance, info);
        
        std::vector<rhi::runtime::PysicalDevicePtr> physicalDevices;
        physicalDevices.reserve(physicalDevices.size());
        for (auto & d : apiPhysicalDevices) {
            physicalDevices.emplace_back(std::make_unique<Api::PhysicalDevice>(std::move(d)));
        }
        return physicalDevices;
    }

    rhi::runtime::DevicePtr Rhi::createDevice(rhi::runtime::PysicalDevicePtr & gpu, const rhi::DeviceCreateInfo & info) const {
        return std::make_unique<Api::Device>(static_cast<Api::PhysicalDevice &>(*gpu), info);
    }

    rhi::runtime::SwapchainPtr Rhi::createSwapchain(const rhi::runtime::DevicePtr & device, 
        const rhi::runtime::SurfacePtr & surface, const rhi::SwapchainCreateInfo & info) const {

        return std::make_unique<Api::Swapchain>(static_cast<const Api::Device &>(*device), 
            static_cast<const Api::Surface &>(*surface), info);
    }

    rhi::runtime::ImagePtr Rhi::createImage(const rhi::runtime::DevicePtr & device, const rhi::ImageInfo & info) const noexcept {
        return std::make_unique<Api::Image>(static_cast<Api::Device &>(*device), info);
    }

    rhi::runtime::RenderPass Rhi::getRenderPass(rhi::runtime::DevicePtr & device, 
        const rhi::RenderPassDescription & description) const noexcept {
        return static_cast<Api::Device &>(*device).getRenderPass(description);
    }

    rhi::runtime::FrameBufferPtr Rhi::createFrameBuffer(
        const rhi::runtime::DevicePtr & device, 
        std::vector<rhi::FrameBufferAttachment> && attachemnts, 
        const rhi::runtime::RenderPass & renderPass) const noexcept {

        return std::make_unique<Api::FrameBuffer>(static_cast<const Api::Device &>(*device), 
        std::move(attachemnts), static_cast<VkRenderPass>(renderPass));
    }

    rhi::runtime::QueuePtr Rhi::getQueue(const rhi::runtime::DevicePtr & device, 
        rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) const {
        return std::make_unique<Api::Queue>(static_cast<const Api::Device &>(*device), familyFlags, queueIdx, framesCount);
    }

    rhi::runtime::Fence Rhi::createFence(const rhi::runtime::DevicePtr & device, 
        rhi::FenceCreateFlags flags) const noexcept {
        return static_cast<const Api::Device &>(*device).createFence(flags);
    }
    rhi::runtime::Semaphore Rhi::createSemaphore(const rhi::runtime::DevicePtr & device) const noexcept {
        return static_cast<const Api::Device &>(*device).createSemaphore();
    }

    void Rhi::destroyFence(const rhi::runtime::DevicePtr & device, 
        const rhi::runtime::Fence & fence) const noexcept {
        static_cast<const Api::Device &>(*device).destroyFence(static_cast<VkFence>(fence));
    }
    void Rhi::destroySemaphore(const rhi::runtime::DevicePtr & device, 
        const rhi::runtime::Semaphore & semaphore) const noexcept {
        static_cast<const Api::Device &>(*device).destroySemaphore(static_cast<VkSemaphore>(semaphore));
    }

    rhi::runtime::ShaderModulesManagerPtr Rhi::createShaderModulesManager() const {
        return std::make_unique<engine::rhi::ShaderModulesManager<Api>>();
    }

    rhi::runtime::GpuProgramPtr Rhi::createProgram(
        rhi::runtime::DevicePtr & device, 
        std::span<const rhi::runtime::ShaderModuleView> moduleViews
    ) const {
        std::array<Api::ShaderModule::View, rhi::shaders::kStagesCount> apiModules;
        size_t i = 0u;
        for (const auto & view : moduleViews) {
            apiModules[i++] = {
                static_cast<VkShaderModule>(view.shaderModule), 
                static_cast<const engine::vulkan::ModuleReflection*>(view.reflection)
            };
        }

        return std::make_unique<Api::GpuProgram>(static_cast<Api::Device &>(*device), 
            std::span{apiModules.begin(), apiModules.begin() + moduleViews.size()});
    }

    std::unique_ptr<engine::rhi::runtime::Pipeline> Rhi::createComputePipeline(
        rhi::runtime::DevicePtr & device,
        const rhi::runtime::GpuProgramPtr & program) const {

        return std::make_unique<Api::Pipeline>(static_cast<Api::Device &>(*device),
            computePipelineCreateInfo(static_cast<const Api::GpuProgram&>(*program)));
    }

    std::unique_ptr<engine::rhi::runtime::Pipeline> Rhi::createGraphicsPipeline(
        rhi::runtime::DevicePtr & device,
        rhi::runtime::RenderPass renderPass,
        const rhi::runtime::GpuProgramPtr & program,
        const rhi::GraphicsPipelineStatesInfo & info) const {

        const Api::GraphicsPipelineInfo pipelineInfo{info};
        return std::make_unique<Api::Pipeline>(static_cast<Api::Device &>(*device), 
            pipelineInfo.create(static_cast<const Api::GpuProgram&>(*program), 
            static_cast<VkRenderPass>(renderPass)));
    }

    rhi::runtime::BufferPtr Rhi::createBuffer(rhi::runtime::DevicePtr & device, 
        const rhi::BufferInfo & info, size_t size) const {
        return std::make_unique<Api::Buffer>(static_cast<Api::Device &>(*device), info, size);
    }
}