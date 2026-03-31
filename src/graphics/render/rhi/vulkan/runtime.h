#pragma once

#include "api.h"
#include "instance.h"
#include "buffer.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/shared_ref.h"

#include "allocators/pool_allocator_chain.h"
#include <vector>

namespace engine::vulkan::runtime {

    class Rhi final : public rhi::runtime::Rhi {
    public:
        using Api = rhi::Api<vulkan::Api>;
        explicit Rhi(const rhi::InstanceCreateInfo & instanceCreateInfo) : _instance(instanceCreateInfo) {}
    
        rhi::runtime::Instance & getInstance() noexcept override { return _instance; }
    
        rhi::runtime::SurfacePtr createSurface(const rhi::SurfaceCreateInfo & info) const override;

        std::vector<rhi::runtime::PysicalDevicePtr> enumerateDevices(const rhi::PhysicalDeviceRequirements & info) const override;
        rhi::runtime::DevicePtr createDevice(rhi::runtime::PysicalDevicePtr & gpu, const rhi::DeviceCreateInfo & info) const override;
    
        rhi::runtime::SwapchainPtr createSwapchain(const rhi::runtime::Device & device, 
            const rhi::runtime::Surface & surface, const rhi::SwapchainCreateInfo & info) const override;
    
        rhi::runtime::ImagePtr createImage(rhi::runtime::Device & device, const rhi::ImageInfo & info) const noexcept override;
    
        rhi::runtime::RenderPass getRenderPass(rhi::runtime::Device & device, 
            const rhi::RenderPassDescription & description) const noexcept override;
    
        rhi::runtime::FrameBufferPtr createFrameBuffer(
            const rhi::runtime::Device & device, 
            std::vector<rhi::FrameBufferAttachment> && attachemnts, 
            const rhi::runtime::RenderPass & renderPass) const noexcept override;
    
        rhi::runtime::QueuePtr getQueue(const rhi::runtime::Device & device, 
            rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) const override;
    
        rhi::runtime::Fence createFence(const rhi::runtime::Device & device, 
            rhi::FenceCreateFlags flags) const noexcept override;

        rhi::runtime::Semaphore createSemaphore(const rhi::runtime::Device & device) const noexcept override;
    
        rhi::runtime::Event createEvent(const rhi::runtime::Device & device, 
            rhi::EventCreateFlags flags) const noexcept override;

        void waitFence(const rhi::runtime::Device & device, 
            const rhi::runtime::Fence & fence, uint64_t timeout) const noexcept override;

        void resetFence(const rhi::runtime::Device & device, 
            const rhi::runtime::Fence & fence) const noexcept override;

        void destroyFence(const rhi::runtime::Device & device, 
            const rhi::runtime::Fence & fence) const noexcept override;

        void destroySemaphore(const rhi::runtime::Device & device, 
            const rhi::runtime::Semaphore & semaphore) const noexcept override;

        void destroyEvent(const rhi::runtime::Device & device, 
            const rhi::runtime::Event & event) const noexcept override;

        void resetEvent(const rhi::runtime::Device & device, const rhi::runtime::Event & event) const noexcept override;
    
        rhi::runtime::ShaderModulesManagerPtr createShaderModulesManager() const override;
    
        rhi::runtime::GpuProgramPtr createProgram(
            rhi::runtime::Device & device, 
            std::span<const rhi::runtime::ShaderModuleView> moduleViews,
            rhi::shaders::SpecializationInfo && specializationInfo = {}
        ) const override;
    
        rhi::runtime::PipelinePtr createComputePipeline(
            rhi::runtime::Device & device,
            const rhi::runtime::GpuProgramPtr & program) const override;
    
        rhi::runtime::PipelinePtr createGraphicsPipeline(
            rhi::runtime::Device & device,
            rhi::runtime::RenderPass renderPass,
            const rhi::runtime::GpuProgramPtr & program,
            const rhi::GraphicsPipelineStatesInfo & info, 
            uint32_t subpass = 0u) const override;
    
        rhi::runtime::BufferPtr createBuffer(rhi::runtime::Device & device, 
            const rhi::BufferInfo & info, size_t size) override; 

        void destroyBuffer(rhi::runtime::BufferPtr & ptr);

        rhi::runtime::CommandListPtr createCommandList(rhi::runtime::CommandPool * commandPool, 
            rhi::CommandBufferLevel level = rhi::CommandBufferLevel::ePrimary) const override;

        rhi::runtime::QueryPoolPtr createQueryPool(rhi::runtime::Device & device, const rhi::QueryPoolInfo & info) const override;
        
    private:
        Api::Instance _instance;

        using BufferComposition = rhi::runtime::sh_ptr<Api::Buffer>::DefaultValueCompositionWithDeleter<decltype([ptr = nullptr](void*){})>;
        static constexpr uint32_t kBufferAlign = alignof(BufferComposition);
        static constexpr uint32_t kBufferPtrSize = sizeof(BufferComposition);

        using BuffersAllocator = allocators::PoolAllocatorChain<kBufferPtrSize, kBufferAlign, true, true>;
        BuffersAllocator _buffersPool{1024u, 32u}; // ~6mb
    };
}