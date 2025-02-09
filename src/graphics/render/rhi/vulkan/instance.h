#pragma once

#include "graphics/render/rhi/common/instance.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace engine::vulkan {

class Instance {
public:
    Instance(const rhi::InstanceCreateInfo & info);
    ~Instance();

    inline explicit operator bool() const noexcept { return _instance != VK_NULL_HANDLE; }
    inline VkInstance operator()() const noexcept { return _instance; }
    
    inline VkInstance vkInstance() const noexcept { return _instance; }
    inline const VkAllocationCallbacks * getAllocationCallbacks() const noexcept { return _allocationCallbacks; }
    
    inline const std::vector<const char*> & getLayers() const noexcept { return _instanceLayers; }

    inline uint32_t getApiVersion() const noexcept { return _apiVersion; }

private:
    uint32_t _apiVersion = 0u;
    VkInstance _instance = VK_NULL_HANDLE;
    const VkAllocationCallbacks * _allocationCallbacks = nullptr;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    std::vector<const char*> _instanceLayers;
};

}