#pragma once

#include "graphics/render/rhi/common/device.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace engine::vulkan {

class Instance;
class Surface;
class PhisicalDevice;

struct SurfaceSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct PhysicalDeviceExtFeatures {
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features = {};
    VkPhysicalDeviceTransformFeedbackFeaturesEXT transformFeedbackFeatures = {};
    VkPhysicalDeviceSynchronization2Features synchronization2Features = {};
};

void createFeatures2Chain(void ** featuresNext, PhysicalDeviceExtFeatures & features2Ext, const rhi::PhysicalDeviceExtFeatures & targetFeatures) noexcept;
SurfaceSupportDetails getSurfaceSupportDetails(const PhisicalDevice & gpu, const Surface & surface) noexcept;

class PhisicalDevice {
public:
    static std::vector<PhisicalDevice> getSuitableDevices(const Instance & instance, const rhi::PhysicalDeviceRequirements & info);

    PhisicalDevice(PhisicalDevice &&) noexcept = default;
    PhisicalDevice(const Instance & instance, VkPhysicalDevice device, const rhi::PhysicalDeviceRequirements & info);

    inline explicit operator bool() const noexcept { return _gpu != VK_NULL_HANDLE; }

    inline const Instance & getInstance() const noexcept {return _instance; }
    inline VkPhysicalDevice gpu() const noexcept { return _gpu; }
    inline VkPhysicalDevice operator()() const noexcept { return _gpu; }

    inline const VkPhysicalDeviceMemoryProperties & memoryProperties() const noexcept { return _memoryProperties; }
    inline const VkPhysicalDeviceProperties & deviceProperties() const noexcept { return _properties; }
    inline const VkPhysicalDeviceFeatures2 & deviceFeatures() const noexcept { return _features; }
    inline const PhysicalDeviceExtFeatures & deviceFeatures2Ext() const noexcept {return _features2Ext; }
    inline PhysicalDeviceExtFeatures copyDeviceFeatures2Ext() const noexcept {return _features2Ext; }
    inline const std::vector<const char*> & deviceExtensions() const noexcept { return _extensions; }
    
    inline std::vector<rhi::QueueFamily> & queues() noexcept { return _queues; }
    inline const std::vector<rhi::QueueFamily> & queues() const noexcept { return _queues; }

    bool checkSurfaceSupport(const Surface & surface) noexcept;
    
    enum class DepthFormatSupport : uint8_t {
        eUnsupport = 0u,
        eOptimalTiling = 1u,
        elinearTiling = 2u,
        eAll = 3u
    };

    DepthFormatSupport isDepthFormatSupported(VkFormat format, bool samplingSupport) const noexcept;
    rhi::Format getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport = false) const noexcept;

    uint32_t getMemoryType(uint32_t requiredTypeBits, rhi::MemoryPropertyFlags properties) const noexcept;

    std::pair<uint32_t, const rhi::QueueFamily*> getQueueFamily(uint8_t flags, uint32_t start = 0u) const noexcept;

private:
    const Instance & _instance;
    VkPhysicalDevice _gpu = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties _memoryProperties;
    VkPhysicalDeviceProperties _properties;
    VkPhysicalDeviceFeatures2 _features = {};
    PhysicalDeviceExtFeatures _features2Ext = {};
    std::vector<const char*> _extensions;
    std::vector<rhi::QueueFamily> _queues;
};

}