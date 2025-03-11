#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "vk_types.h"

#include "log/log.h"

#include <algorithm>
#include <vector>

namespace engine::vulkan {

    Device::Device(const PhisicalDevice & gpu, const rhi::DeviceCreateInfo & info) : 
        _gpu(gpu), 
        _commandPoolManager(*this, info.commandPoolThreadsCount) {

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
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

        createAllocator();
    }

    Device::~Device() {
        destroyAllocator();

        if (_device) {
            vkDestroyDevice(_device, _gpu.getInstance().getAllocationCallbacks());
            _device = VK_NULL_HANDLE;
            LOG_I("vulkan: device destroyed");
        }
    }

    void Device::createAllocator() noexcept {
        const auto & instance = _gpu.getInstance();

        AllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.apiVersion = instance.getApiVersion();
        allocatorCreateInfo.allocationCallbacks = instance.getAllocationCallbacks();
        
        //  fill allocatorCreateInfo flags
        const auto & gpuExtensionNames = _gpu.deviceExtensions();

        for (const auto ext : gpuExtensionNames) {
            if (strcmp(ext, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
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

    const DeviceQueueFamily * Device::findQueueFamily(uint8_t flags, uint32_t start) const noexcept {
        if (auto it = std::find_if(_queueFamilies.begin() + start, _queueFamilies.end(), [flags](auto const & queue) {
            return (queue.flags & flags) == flags;
        }); it != _queueFamilies.end()) {
            return &(*it);
        }
        return nullptr;
    }

    VkFence Device::createFence(rhi::FenceCreateFlags flags) noexcept {
        VkFenceCreateInfo fenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(flags)
        };

        VkFence result;
        vkCreateFence(_device, &fenceCreateInfo, _gpu.getInstance().getAllocationCallbacks(), &result);
        return result;
    }

    VkSemaphore Device::createSemaphore() noexcept {
        VkSemaphoreCreateInfo semaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkSemaphore result;
        vkCreateSemaphore(_device, &semaphoreCreateInfo, _gpu.getInstance().getAllocationCallbacks(), &result);
        return result;
    }

}
