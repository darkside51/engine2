#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "vk_types.h"

#include "utils/dynamic_array.h"
#include "log/log.h"

#include <algorithm>
#include <vector>

namespace engine::vulkan {

    Device::Device(const PhysicalDevice & gpu, const rhi::DeviceCreateInfo & info) : 
        _gpu(gpu),
        _inFlightFramesCount(info.inFlightFramesCount), 
        _commandPoolManager(*this, info.commandPoolThreadsCount),
        _renderpassManager(*this),
        _descriptorSetLayoutManager(*this),
        _descriptorPoolManager(*this),
        _samplerManager(*this) {

        stl::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(info.queueFamilies.size());
        for (const auto & family : info.queueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = family.index;
            queueCreateInfo.queueCount = family.queueCount;
            queueCreateInfo.pQueuePriorities = family.priorities.data();
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures2 features;
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = nullptr; //
        features.features = utils::convert2vk(info.featureRequest.features);

        // features extensions
        auto features2Ext = _gpu.copyDeviceFeatures2Ext();
        createFeatures2Chain(&features.pNext, features2Ext, info.featureRequest.extFeatures);

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr;

        const auto & extensions =_gpu.deviceExtensions();
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        const auto & instance = _gpu.getInstance();
        const auto & layers = instance.getLayers();

        createInfo.enabledLayerCount = layers.size();
        createInfo.ppEnabledLayerNames = layers.data();
        createInfo.pNext = &features;

        if (auto result = vkCreateDevice(_gpu(), &createInfo, instance.getAllocationCallbacks(), &_device); result != VK_SUCCESS) {
            LOG_E("vkCreateDevice error: %d", result);
            _device = VK_NULL_HANDLE;
            return;
        }

        _queueFamilies.resize(info.queueFamilies.size());
        uint32_t i = 0u;
        for (auto & family : _queueFamilies) {
            const auto & srcFamily = info.queueFamilies[i];
            family.index = srcFamily.index;
            family.flags = srcFamily.flags;
            family.queues.resize(srcFamily.queueCount);
            for (uint32_t j = 0u; j < srcFamily.queueCount; ++j) {
                vkGetDeviceQueue(_device, family.index, j, &family.queues[j]);
            }
            ++i;
        }

        LOG_I("vulkan: device create successful");
        constexpr auto deviceTypeName = [](VkPhysicalDeviceType type) {
            switch (type) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "Other";
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated_GPU";
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete_GPU";
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual_GPU";
                case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
            }
            return "Unknown";
        };

        const auto & properties = _gpu.deviceProperties();
        LOG_I("vulkan: gpu name: %s, type: %s", properties.deviceName, deviceTypeName(properties.deviceType));
        LOG_I("vulkan: api version: (%u, %u, %u), gpu driver vesion: %u", 
            VK_API_VERSION_MAJOR(properties.apiVersion), 
            VK_API_VERSION_MINOR(properties.apiVersion), 
            VK_API_VERSION_PATCH(properties.apiVersion), 
            properties.driverVersion);

        createPipelineCache();
        createAllocator(info.largeHeapBlockSize);
    }

    Device::~Device() {
        waitIdle();
        _renderpassManager.destroy();
        _commandPoolManager.destroy();
        _descriptorPoolManager.destroy();
        _descriptorSetLayoutManager.destroy();
        _samplerManager.destroy();

        destroyPipelineCache();
        destroyAllocator();
        
        if (_device) {
            vkDestroyDevice(_device, _gpu.getInstance().getAllocationCallbacks());
            _device = VK_NULL_HANDLE;
            LOG_I("vulkan: device destroyed");
        }
    }

    rhi::Format Device::getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport) const noexcept {
        return _gpu.getSupportedDepthFormat(desiredBpp, withStencil, samplingSupport);
    }

    void Device::createPipelineCache() noexcept {
        const VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .initialDataSize = 0u,
            .pInitialData = nullptr
        };
		vkCreatePipelineCache(_device, &pipelineCacheCreateInfo, _gpu.getInstance().getAllocationCallbacks(), &_pipelineCache);
    }

    void Device::destroyPipelineCache() noexcept {
        vkDestroyPipelineCache(_device, _pipelineCache, _gpu.getInstance().getAllocationCallbacks());
    }

    void Device::createAllocator(uint32_t largeHeapBlockSize) noexcept {
        const auto & instance = _gpu.getInstance();
        const auto apiVersion = instance.getApiVersion();

        AllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.apiVersion = apiVersion;
        allocatorCreateInfo.allocationCallbacks = instance.getAllocationCallbacks();
        allocatorCreateInfo.preferredLargeHeapBlockSize = largeHeapBlockSize;
        
        //  fill allocatorCreateInfo flags
        const auto & gpuExtensionNames = _gpu.deviceExtensions();
        
        for (const auto ext : gpuExtensionNames) {
            if (VK_VERSION_MAJOR(apiVersion) >= 1 && VK_VERSION_MINOR(apiVersion) >= 1 && 
                strcmp(ext, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.KHR_dedicated_allocation_enabled = true;
            } else if (strcmp(ext, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.KHR_bind_memory2_enabled = true;
            } else if (strcmp(ext, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.EXT_memory_budget_enabled = true;
            } else if (strcmp(ext, VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.AMD_device_coherent_memory_enabled = true;
            } else if (strcmp(ext, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.KHR_buffer_device_address_enabled = true;
            } else if (strcmp(ext, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.EXT_memory_priority_enabled = true;
            } 
            #ifdef VK_KHR_MAINTENANCE_5_EXTENSION_NAME
            else if (strcmp(ext, VK_KHR_MAINTENANCE_5_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.KHR_maintenance5_enabled = true;
            }
            #endif
            #ifdef VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
            else if (strcmp(ext, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) == 0) {
                allocatorCreateInfo.flags.KHR_external_memory_win32_enabled = true;
            }
            #endif
        }

        _memoryAllocator.create(instance(), _gpu(), _device, allocatorCreateInfo);
    }

    void Device::destroyAllocator() noexcept {
        _memoryAllocator.destroy();
    }

    const DeviceQueueFamily * Device::findQueueFamily(rhi::queue_flags::flags flags, uint32_t start) const noexcept {
        if (auto it = std::find_if(_queueFamilies.begin() + start, _queueFamilies.end(), [flags](auto const & queue) {
            return (queue.flags & flags) == flags;
        }); it != _queueFamilies.end()) {
            return &(*it);
        }
        return nullptr;
    }

    VkFence Device::createFence(rhi::FenceCreateFlags flags) const noexcept {
        const VkFenceCreateInfo fenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(flags)
        };

        VkFence result;
        vkCreateFence(_device, &fenceCreateInfo, _gpu.getInstance().getAllocationCallbacks(), &result);
        return result;
    }

    VkSemaphore Device::createSemaphore() const noexcept {
        const VkSemaphoreCreateInfo semaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkSemaphore result;
        vkCreateSemaphore(_device, &semaphoreCreateInfo, _gpu.getInstance().getAllocationCallbacks(), &result);
        return result;
    }

    VkEvent Device::createEvent(rhi::EventCreateFlags flags) const noexcept {
        const VkEventCreateInfo eventCreateInfo {
            .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(flags)
        };

        VkEvent result;
        vkCreateEvent(_device, &eventCreateInfo, _gpu.getInstance().getAllocationCallbacks(), &result);
        return result;
    }

    void Device::waitForFence(const Fence & fence, uint64_t timeout) const noexcept {
        vkWaitForFences(_device, 1, &fence, VK_TRUE, timeout);
    }

    void Device::resetFence(const Fence & fence) const noexcept {
        vkResetFences(_device, 1, &fence);
    }

    void Device::destroyFence(const Fence & fence) const noexcept {
        vkResetFences(_device, 1, &fence);
        vkDestroyFence(_device, fence, _gpu.getInstance().getAllocationCallbacks());
    }

    void Device::destroySemaphore(const Semaphore & semaphore) const noexcept {
        vkDestroySemaphore(_device, semaphore, _gpu.getInstance().getAllocationCallbacks());
    }

    void Device::destroyEvent(const Event & event) const noexcept {
        vkDestroyEvent(_device, event, _gpu.getInstance().getAllocationCallbacks());
    }

    void Device::resetEvent(const Event & event) const noexcept {
        vkResetEvent(_device, event);
    }

    VkDescriptorSetLayout Device::createDescriptorSetLayout(
        const rhi::shaders::BindingSetLayout & layout) const noexcept {
        using array_type = dynamic_array<VkDescriptorSetLayoutBinding, rhi::shaders::BindingSetLayout::kMaxBindingsInSetOnStack>;
        array_type bindings{layout.layouts.size()};

        const auto bindingsView = bindings.view();
        const auto layoutsView = layout.layouts.view();

        for (size_t i = 0; i < layoutsView.size(); ++i) {
            const auto & l = layoutsView[i];
            bindingsView[i] = VkDescriptorSetLayoutBinding {
                .binding = l.binding,
                .descriptorType = utils::convert2vk(l.type),
                .descriptorCount = l.count,
                .stageFlags = utils::convert2vk(l.shaderStages),
                .pImmutableSamplers = nullptr
            };
        }

        VkDescriptorSetLayoutCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(layout.flags),
            .bindingCount = static_cast<uint32_t>(layout.layouts.size()),
            .pBindings = bindings.data()
        };

        VkDescriptorSetLayout descriptorSetLayout;
        if (vkCreateDescriptorSetLayout(_device, &info, _gpu.getInstance().getAllocationCallbacks(),
            &descriptorSetLayout) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }

        return descriptorSetLayout;
    }

    void Device::destroyDescriptorSetLayout(VkDescriptorSetLayout layout) const noexcept {
        vkDestroyDescriptorSetLayout(_device, layout, _gpu.getInstance().getAllocationCallbacks());
    }

    size_t Device::getUniformBufferOffsetAlignment() const noexcept {
        return _gpu.deviceProperties().limits.minUniformBufferOffsetAlignment;
    }

    size_t Device::getStorageBufferOffsetAlignment() const noexcept {
        return _gpu.deviceProperties().limits.minStorageBufferOffsetAlignment;
    }

    size_t Device::getTexelBufferOffsetAlignment() const noexcept {
        return _gpu.deviceProperties().limits.minTexelBufferOffsetAlignment;
    }

    float Device::getTimestampPeriod() const noexcept {
        return _gpu.deviceProperties().limits.timestampPeriod;
    }

    rhi::FormatFeatureAccess Device::isFormatSupported(rhi::Format format, rhi::FormatFeatureFlags featureFlags) const noexcept {
        return _gpu.isFormatSupported(utils::convert2vk(format), utils::convert2vk(featureFlags));
    }

    bool Device::isExtensionSupported(std::string_view extension) const noexcept {
        const auto & extensions = _gpu.deviceExtensions();
        auto it = std::find_if(extensions.begin(), extensions.end(), [extension](const auto & ext){
            return extension == ext;
        });
        return it != extensions.end();
    }

    Device::Sampler Device::getSampler(const rhi::SamplerInfo & samplerInfo) {
        return _samplerManager.getSampler(samplerInfo);
    }

    void Device::evaluateSampler(rhi::SamplerValue & samplerValue) {
        samplerValue.evaluatedSampler = getSampler(samplerValue.info);
    }
}
