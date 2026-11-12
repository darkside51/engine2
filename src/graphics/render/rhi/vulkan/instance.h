#pragma once

#include "graphics/render/rhi/common/instance.h"
#include "graphics/render/rhi/common/runtime.h"
#include <vulkan/vulkan.h>

namespace engine::vulkan {

class Instance final : public rhi::runtime::Instance {
public:
    Instance(const rhi::InstanceCreateInfo & info);
    ~Instance();

    inline explicit operator bool() const noexcept { return _instance != VK_NULL_HANDLE; }
    inline VkInstance operator()() const noexcept { return _instance; }
    
    inline VkInstance vkInstance() const noexcept { return _instance; }
    inline const VkAllocationCallbacks * getAllocationCallbacks() const noexcept { return _allocationCallbacks; }
    
    inline const stl::vector<const char*> & getLayers() const noexcept { return _instanceLayers; }

    inline uint32_t getApiVersion() const noexcept { return _apiVersion; }

private:
    uint32_t _apiVersion = 0u;
    VkInstance _instance = VK_NULL_HANDLE;
    const VkAllocationCallbacks * _allocationCallbacks = nullptr;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    stl::vector<const char*> _instanceLayers;
};

}