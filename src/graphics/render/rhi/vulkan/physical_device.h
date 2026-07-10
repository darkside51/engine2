#pragma once

#include "graphics/render/rhi/common/device.h"
#include "graphics/render/rhi/common/runtime.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace engine::vulkan {

class Instance;
class Surface;
class PhysicalDevice;

struct SurfaceSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    stl::vector<VkSurfaceFormatKHR> formats;
    stl::vector<VkPresentModeKHR> presentModes;
};

struct PhysicalDeviceExtFeatures {
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features = {};
    VkPhysicalDeviceTransformFeedbackFeaturesEXT transformFeedbackFeatures = {};
    VkPhysicalDeviceSynchronization2Features synchronization2Features = {};
};

void createFeatures2Chain(void ** featuresNext, PhysicalDeviceExtFeatures & features2Ext, const rhi::PhysicalDeviceExtFeatures & targetFeatures) noexcept;
SurfaceSupportDetails getSurfaceSupportDetails(const PhysicalDevice & gpu, const Surface & surface) noexcept;

class PhysicalDevice final : public rhi::runtime::PhysicalDevice {
public:
    static stl::vector<PhysicalDevice> getSuitableDevices(const Instance & instance, const rhi::PhysicalDeviceRequirements & info);

    PhysicalDevice(PhysicalDevice &&) noexcept = default;
    PhysicalDevice(const Instance & instance, VkPhysicalDevice device, const rhi::PhysicalDeviceRequirements & info);

    inline explicit operator bool() const noexcept { return _gpu != VK_NULL_HANDLE; }

    inline const Instance & getInstance() const noexcept {return *_instance; }
    inline VkPhysicalDevice gpu() const noexcept { return _gpu; }
    inline VkPhysicalDevice operator()() const noexcept { return _gpu; }

    inline const VkPhysicalDeviceMemoryProperties & memoryProperties() const noexcept { return _memoryProperties; }
    inline const VkPhysicalDeviceProperties & deviceProperties() const noexcept { return _properties; }
    inline const VkPhysicalDeviceFeatures2 & deviceFeatures() const noexcept { return _features; }
    inline const PhysicalDeviceExtFeatures & deviceFeatures2Ext() const noexcept {return _features2Ext; }
    inline PhysicalDeviceExtFeatures copyDeviceFeatures2Ext() const noexcept {return _features2Ext; }
    inline const stl::vector<const char*> & deviceExtensions() const noexcept { return _extensions; }
    
    inline stl::vector<rhi::QueueFamily> & queues() noexcept { return _queues; }
    inline const stl::vector<rhi::QueueFamily> & queues() const noexcept { return _queues; }

    bool checkSurfaceSupport(const rhi::runtime::Surface & surface) noexcept override;

    bool checkSurfaceSupport(const Surface & surface) noexcept;
    
    enum class DepthFormatSupport : uint8_t {
        eUnsupport = 0u,
        eOptimalTiling = 1u,
        elinearTiling = 2u,
        eAll = 3u
    };

    rhi::FormatFeatureAccess isFormatSupported(VkFormat format, VkFormatFeatureFlags featureFlags) const noexcept;

    DepthFormatSupport isDepthFormatSupported(VkFormat format, bool samplingSupport) const noexcept;
    rhi::Format getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport = false) const noexcept;

    uint32_t getMemoryType(uint32_t requiredTypeBits, rhi::MemoryPropertyFlags properties) const noexcept;

    stl::pair<uint32_t, const rhi::QueueFamily*> getQueueFamily(uint8_t flags, uint32_t start = 0u) const noexcept override;
    float timestampToMilliseconds(double time) const noexcept override;

private:
    const Instance * _instance = nullptr;
    VkPhysicalDevice _gpu = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties _memoryProperties;
    VkPhysicalDeviceProperties _properties;
    VkPhysicalDeviceFeatures2 _features = {};
    PhysicalDeviceExtFeatures _features2Ext = {};
    stl::vector<const char*> _extensions;
    stl::vector<rhi::QueueFamily> _queues;
};

}