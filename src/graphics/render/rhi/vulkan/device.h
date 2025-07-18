#pragma once

#include "graphics/render/rhi/common/device.h"
#include "graphics/render/rhi/common/runtime.h"
#include "memory_allocator.h"
#include "command_pool.h"
#include "renderpass.h"
#include "descriptor_set_layout_manager.h"
#include "descriptor_pool_manager.h"
#include "sampler_manager.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace engine::vulkan {

class Api;
class PhysicalDevice;

struct DeviceQueueFamily {
    uint32_t index = 0u;
    std::vector<VkQueue> queues;
    uint8_t flags = 0u;
};

class Device final : public rhi::runtime::Device {
public:
    using Api = vulkan::Api;
    // using RenderPass = VkRenderPass;
    using Fence = VkFence;
    using Semaphore = VkSemaphore;
    using Event = VkEvent;
    using Sampler = VkSampler;
    
    Device(const PhysicalDevice & gpu, const rhi::DeviceCreateInfo & info);
    ~Device();

    inline explicit operator bool() const noexcept { return _device != VK_NULL_HANDLE; }
    inline VkDevice operator ()() const noexcept { return _device; }

    inline void waitIdle() const noexcept override {
        if (_device) {
            vkDeviceWaitIdle(_device);
        }
    }

    inline const PhysicalDevice & gpu() const noexcept { return _gpu; }

    rhi::Format getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport = false) const noexcept override;

    const std::vector<DeviceQueueFamily> & getQueueFamilies() const noexcept { return _queueFamilies; }
    const DeviceQueueFamily * findQueueFamily(rhi::queue_flags::flags flags, uint32_t start = 0u) const noexcept;
    
    const GPUMemoryAllocator & getMemoryAllocator() const noexcept { return _memoryAllocator; }
    GPUMemoryAllocator & getMemoryAllocator() noexcept { return _memoryAllocator; }

    CommandPool * acquireCommandPool(rhi::queue_flags::flags queueFlags, 
        rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer) noexcept override {
        return _commandPoolManager.acquirePool(queueFlags, poolFlags);
    }
    CommandPool * acquireCommandPool(uint32_t idx, rhi::queue_flags::flags queueFlags, 
        rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer) noexcept {
        return _commandPoolManager.acquirePool(idx, queueFlags, poolFlags);
    }

    RenderPass* getRenderPass(const rhi::RenderPassDescription & description) {
        return _renderpassManager.getRenderPass(description);
    }

    Fence createFence(rhi::FenceCreateFlags flags) const noexcept;
    Semaphore createSemaphore() const noexcept;
    Event createEvent(rhi::EventCreateFlags flags) const noexcept;

    void destroyFence(const Fence & fence) const noexcept;
    void destroySemaphore(const Semaphore & semaphore) const noexcept;
    void destroyEvent(const Event & event) const noexcept;

    void resetEvent(const Event & event) const noexcept;

    Sampler getSampler(const rhi::SamplerInfo & samplerInfo);

    VkDescriptorSetLayout createDescriptorSetLayout(const rhi::shaders::BindingSetLayout & layout) const noexcept;
    void destroyDescriptorSetLayout(VkDescriptorSetLayout layout) const noexcept;

    DescriptorSetLayoutManager & getDescriptorSetLayoutManager() noexcept {
        return _descriptorSetLayoutManager;
    }

    DescriptorPoolManager & getDescriptorPoolManager() noexcept {
        return _descriptorPoolManager;
    }

    VkPipelineCache pipelineCache() const noexcept {
        return _pipelineCache;
    }

    void resetFrameTmpData(uint8_t frameId) noexcept override {
        _descriptorPoolManager.resetPools(frameId); // reset temp pools
    }
    
    uint8_t inFlightFramesCount() const noexcept override { return _inFlightFramesCount; }

    // support
    size_t getUniformBufferOffsetAlignment() const noexcept override;
    size_t getStorageBufferOffsetAlignment() const noexcept override;
    size_t getTexelBufferOffsetAlignment() const noexcept override;

    float getTimestampPeriod() const noexcept;

    rhi::FormatFeatureAccess isFormatSupported(rhi::Format format, rhi::FormatFeatureFlags featureFlags) const noexcept override;

private:
    void createAllocator() noexcept;
    void destroyAllocator() noexcept;

    void createPipelineCache() noexcept;
    void destroyPipelineCache() noexcept;

    VkDevice _device = VK_NULL_HANDLE;
    const PhysicalDevice & _gpu;
    std::vector<DeviceQueueFamily> _queueFamilies;
    uint8_t _inFlightFramesCount = 3u;

    GPUMemoryAllocator _memoryAllocator;
    CommandPoolManager _commandPoolManager;
    RenderPassManager _renderpassManager; // no sync
    DescriptorSetLayoutManager _descriptorSetLayoutManager; // no sync
    DescriptorPoolManager _descriptorPoolManager; // no sync
    SamplerManager _samplerManager; // no sync
    VkPipelineCache _pipelineCache = VK_NULL_HANDLE;
};

}