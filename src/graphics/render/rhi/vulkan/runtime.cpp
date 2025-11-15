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
#include "image.h"
#include "command_list.h"
#include "command_buffer.h"
#include "renderpass.h"
#include "query_pool.h"
#include "vk_types.h"

#include "graphics/render/rhi/common/shader_module.h"

#include <array>
#include <cassert>
#include <utility>

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

    rhi::runtime::SwapchainPtr Rhi::createSwapchain(const rhi::runtime::Device & device, 
        const rhi::runtime::Surface & surface, const rhi::SwapchainCreateInfo & info) const {

        return std::make_unique<Api::Swapchain>(static_cast<const Api::Device &>(device), 
            static_cast<const Api::Surface &>(surface), info);
    }

    rhi::runtime::ImagePtr Rhi::createImage(rhi::runtime::Device & device, const rhi::ImageInfo & info) const noexcept {
        return rhi::runtime::sh_ptr<Api::Image>::make(static_cast<Api::Device &>(device), info);
        // return std::make_unique<Api::Image>(static_cast<Api::Device &>(*device), info);
    }

    rhi::runtime::RenderPass Rhi::getRenderPass(rhi::runtime::Device & device, 
        const rhi::RenderPassDescription & description) const noexcept {
        return static_cast<Api::Device &>(device).getRenderPass(description);
    }

    rhi::runtime::FrameBufferPtr Rhi::createFrameBuffer(
        const rhi::runtime::Device & device, 
        std::vector<rhi::FrameBufferAttachment> && attachemnts, 
        const rhi::runtime::RenderPass & renderPass) const noexcept {

        return rhi::runtime::sh_ptr<Api::FrameBuffer>::make(static_cast<const Api::Device &>(device), 
            std::move(attachemnts), static_cast<RenderPass*>(renderPass)->vkRenderPass);

        // return std::make_unique<Api::FrameBuffer>(static_cast<const Api::Device &>(*device), 
        //     std::move(attachemnts), static_cast<VkRenderPass>(renderPass));
    }

    rhi::runtime::QueuePtr Rhi::getQueue(const rhi::runtime::Device & device, 
        rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) const {
        return std::make_unique<Api::Queue>(static_cast<const Api::Device &>(device), familyFlags, queueIdx, framesCount);
    }

    rhi::runtime::Fence Rhi::createFence(const rhi::runtime::Device & device, 
        rhi::FenceCreateFlags flags) const noexcept {
        return static_cast<const Api::Device &>(device).createFence(flags);
    }
    rhi::runtime::Semaphore Rhi::createSemaphore(const rhi::runtime::Device & device) const noexcept {
        return static_cast<const Api::Device &>(device).createSemaphore();
    }
    rhi::runtime::Event Rhi::createEvent(const rhi::runtime::Device & device, rhi::EventCreateFlags flags) const noexcept {
        return static_cast<const Api::Device &>(device).createEvent(flags);
    }

    void Rhi::destroyFence(const rhi::runtime::Device & device, 
        const rhi::runtime::Fence & fence) const noexcept {
        static_cast<const Api::Device &>(device).destroyFence(static_cast<VkFence>(fence));
    }
    void Rhi::destroySemaphore(const rhi::runtime::Device & device, 
        const rhi::runtime::Semaphore & semaphore) const noexcept {
        static_cast<const Api::Device &>(device).destroySemaphore(static_cast<VkSemaphore>(semaphore));
    }
    void Rhi::destroyEvent(const rhi::runtime::Device & device, 
        const rhi::runtime::Event & event) const noexcept {
        static_cast<const Api::Device &>(device).destroyEvent(static_cast<VkEvent>(event));
    }

    void Rhi::resetEvent(const rhi::runtime::Device & device, const rhi::runtime::Event & event) const noexcept {
        static_cast<const Api::Device &>(device).resetEvent(static_cast<VkEvent>(event));
    }

    rhi::runtime::ShaderModulesManagerPtr Rhi::createShaderModulesManager() const {
        return std::make_unique<engine::rhi::ShaderModulesManager<Api>>();
    }

    rhi::runtime::GpuProgramPtr Rhi::createProgram(
        rhi::runtime::Device & device, 
        std::span<const rhi::runtime::ShaderModuleView> moduleViews,
        rhi::shaders::SpecializationInfo && specializationInfo
    ) const {
        std::array<Api::ShaderModule::View, rhi::shaders::kStagesCount> apiModules;
        size_t i = 0u;
        for (const auto & view : moduleViews) {
            apiModules[i++] = {
                static_cast<VkShaderModule>(view.shaderModule), 
                static_cast<const engine::vulkan::ModuleReflection*>(view.reflection)
            };
        }

        return std::make_unique<Api::GpuProgram>(static_cast<Api::Device &>(device), 
            std::span{apiModules.begin(), apiModules.begin() + moduleViews.size()}, std::move(specializationInfo));
    }

    std::unique_ptr<engine::rhi::runtime::Pipeline> Rhi::createComputePipeline(
        rhi::runtime::Device & device,
        const rhi::runtime::GpuProgramPtr & program) const {

        const auto & vkProgram = static_cast<const Api::GpuProgram&>(*program);

        return std::make_unique<Api::Pipeline>(static_cast<Api::Device &>(device), vkProgram,
            computePipelineCreateInfo(vkProgram));
    }

    engine::rhi::runtime::PipelinePtr Rhi::createGraphicsPipeline(
        rhi::runtime::Device & device,
        rhi::runtime::RenderPass renderPass,
        const rhi::runtime::GpuProgramPtr & program,
        const rhi::GraphicsPipelineStatesInfo & info, 
        uint32_t subpass) const {

        const auto & vkProgram = static_cast<const Api::GpuProgram&>(*program);
        const Api::GraphicsPipelineInfo pipelineInfo{info};
        return std::make_unique<Api::Pipeline>(static_cast<Api::Device &>(device), vkProgram,
            pipelineInfo.create(vkProgram, static_cast<RenderPass*>(renderPass)->vkRenderPass, subpass));
    }

    rhi::runtime::BufferPtr Rhi::createBuffer(rhi::runtime::Device & device, 
        const rhi::BufferInfo & info, size_t size) {
        using BufferPtr = rhi::runtime::BufferPtr;
        
        auto * mem = _buffersPool.alloc();
        auto * objectMem = reinterpret_cast<std::byte*>(mem) + align_value(sizeof(BufferPtr::ControlBlock), alignof(Api::Buffer));
        auto * buffer = new(objectMem) Api::Buffer(static_cast<Api::Device &>(device), info, size);

        return rhi::runtime::BufferPtr(mem, alignof(Api::Buffer), sizeof(Api::Buffer), [this](auto * ptr){ destroyBuffer(*static_cast<rhi::runtime::BufferPtr*>(ptr)); });
    }

    void Rhi::destroyBuffer(rhi::runtime::BufferPtr & ptr) {
        using BufferPtr = rhi::runtime::BufferPtr;
        auto * objectMem = reinterpret_cast<std::byte*>(ptr.memory()) + align_value(sizeof(BufferPtr::ControlBlock), alignof(Api::Buffer));
        reinterpret_cast<Api::Buffer*>(objectMem)->~Buffer();
        _buffersPool.free(ptr.memory());
    }

    rhi::runtime::CommandListPtr Rhi::createCommandList(rhi::runtime::CommandPool * commandPool, 
        rhi::CommandBufferLevel level) const {
        return std::make_unique<CommandList>(commandPool, level); 
    }

    rhi::runtime::QueryPoolPtr Rhi::createQueryPool(rhi::runtime::Device & device, const rhi::QueryPoolInfo & info) const {
        return std::make_unique<QueryPool>(static_cast<Api::Device &>(device), info);
    }
}