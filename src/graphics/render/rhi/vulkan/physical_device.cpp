#include "physical_device.h"

#include "instance.h"
#include "helper.h"
#include "surface.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <utility>

namespace engine::vulkan {
    
    namespace {
        static constexpr stl::array<uint8_t, 5> deviceTypePrioritets = {
            4u, // VK_PHYSICAL_DEVICE_TYPE_OTHER = 0
            1u, // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1
            0u, // VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2
            3u, // VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3
            2u  // VK_PHYSICAL_DEVICE_TYPE_CPU = 4
        };

        static constexpr stl::array<uint8_t, 4u> kAllQueueFlags = {
            rhi::queue_flags::graphics,
            rhi::queue_flags::compute,
            rhi::queue_flags::transfrer,
            rhi::queue_flags::sparse_binding
        };
    }

    void createFeatures2Chain(void ** featuresNext, PhysicalDeviceExtFeatures & features2Ext, const rhi::PhysicalDeviceExtFeatures & targetFeatures) noexcept {
        if (targetFeatures.descriptorIndexingFeatures) {
            *featuresNext = &features2Ext.descriptorIndexingFeatures;
            featuresNext = &features2Ext.descriptorIndexingFeatures.pNext;
        }

        if (targetFeatures.shaderFloat16Int8Features) {
            *featuresNext = &features2Ext.shaderFloat16Int8Features;
            featuresNext = &features2Ext.shaderFloat16Int8Features.pNext;
        }

        if (targetFeatures.transformFeedbackFeatures) {
            *featuresNext = &features2Ext.transformFeedbackFeatures;
            featuresNext = &features2Ext.transformFeedbackFeatures.pNext;
        }

        if (targetFeatures.synchronization2Features) {
            *featuresNext = &features2Ext.synchronization2Features;
            featuresNext = &features2Ext.synchronization2Features.pNext;
        }
    }

    SurfaceSupportDetails getSurfaceSupportDetails(const PhysicalDevice & gpu, const Surface & surface) noexcept {
        auto vkSurface = surface.getVkSurface();
        auto vkGpu = gpu();
        SurfaceSupportDetails surfaceSupportDetails;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkGpu, vkSurface, &surfaceSupportDetails.capabilities);
        helper::getVulkanPhysicalDeviceSurfaceFormats(vkGpu, vkSurface, surfaceSupportDetails.formats);
        helper::getVulkanPhysicalDevicePresentModes(vkGpu, vkSurface, surfaceSupportDetails.presentModes);
        return surfaceSupportDetails;
    }

    stl::vector<PhysicalDevice> PhysicalDevice::getSuitableDevices(const Instance & instance, const rhi::PhysicalDeviceRequirements & info) {
        stl::vector<VkPhysicalDevice> devices;
        helper::getVulkanPhysicalDevices(instance.vkInstance(), devices);

        if (devices.empty()) {
            return {};
        }

        stl::vector<PhysicalDevice> result;
        result.reserve(devices.size());

        for (auto & device : devices) {
            PhysicalDevice gpu{instance, device, info};
            if (gpu) {
                result.push_back(std::move(gpu));
            }
        }

        result.shrink_to_fit();
        return result;
    }

    PhysicalDevice::PhysicalDevice(const Instance & instance, VkPhysicalDevice device, const rhi::PhysicalDeviceRequirements & info) :
    _instance(&instance) {
        const auto checkFeature = [](bool required, VkBool32 checked) noexcept -> bool {
            return (required & checked) == required;
        };

        const auto checkFeatures = [&](const rhi::PhysicalDeviceFeatures & required, const VkPhysicalDeviceFeatures & features) noexcept -> bool {
            const uint32_t featuresCount = sizeof(rhi::PhysicalDeviceFeatures) / sizeof(bool);
			for (uint8_t i = 0u; i < featuresCount; ++i) {
                const bool requestFeature = *(reinterpret_cast<const bool*>(&(required)) + i);
                const VkBool32 checkedFeature = *(reinterpret_cast<const VkBool32*>(&(features)) + i);
                if (!checkFeature(requestFeature, checkedFeature)) {
                    return false;
                }
            }

            return true;
        };

        bool suitableDevice = true;

        // check queues
        stl::vector<VkQueueFamilyProperties> deviceQueueFamilyProperties;
        helper::getVulkanPhysicalDeviceQueueFamilyProperties(device, deviceQueueFamilyProperties);

        uint8_t queueFlags = 0u;
        _queues.reserve(deviceQueueFamilyProperties.size());
        for (const VkQueueFamilyProperties & family : deviceQueueFamilyProperties) {
            if (family.queueCount > 0) {
                uint8_t flags = 0u;
                flags |= (family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
                flags |= (family.queueFlags & VK_QUEUE_COMPUTE_BIT);
                flags |= (family.queueFlags & VK_QUEUE_TRANSFER_BIT) ;
                flags |= (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT);

                _queues.emplace_back(flags, family.queueCount, family.timestampValidBits, 
                    rhi::Extent3D {family.minImageTransferGranularity.width, family.minImageTransferGranularity.height, family.minImageTransferGranularity.depth}
                );
                queueFlags |= flags;
            }
        }
        _queues.shrink_to_fit();

        for (uint8_t f : kAllQueueFlags) {
            if ((info.requiredQueueFlags & f) > (queueFlags & f)) {
                suitableDevice = false;
                break;
            }
        }

        if (!suitableDevice) {
            return;
        }

        // check features
        _features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        _features.pNext = nullptr;

        // features extensions
        _features2Ext.descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        _features2Ext.descriptorIndexingFeatures.pNext = nullptr;

        _features2Ext.shaderFloat16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
        _features2Ext.shaderFloat16Int8Features.pNext = nullptr;

        _features2Ext.transformFeedbackFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
        _features2Ext.transformFeedbackFeatures.pNext = nullptr;

        _features2Ext.synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        _features2Ext.synchronization2Features.pNext = nullptr;

        createFeatures2Chain(&_features.pNext, _features2Ext, info.featureRequest.extFeatures);

        vkGetPhysicalDeviceFeatures2(device, &_features);
        if (!checkFeatures(info.featureRequest.features, _features.features)) {
            return;
        }

        // check extensions
        stl::vector<VkExtensionProperties> deviceExtensions;
        helper::getVulkanPhysicalDeviceExtensionProperties(device, deviceExtensions);

        for (const auto & reqExt : info.requiredExtensions) {
            auto it = std::find_if(deviceExtensions.begin(), deviceExtensions.end(), [&reqExt](const auto & ext) {
                return std::strcmp(ext.extensionName, reqExt) == 0;
            });
            if (it == deviceExtensions.end()) {
                suitableDevice = false;
                break;
            }
        }

        if (!suitableDevice) {
            return;
        }

        // properties
        vkGetPhysicalDeviceProperties(device, &_properties);

        if (info.requiredDeviceType && static_cast<uint8_t>(_properties.deviceType) != static_cast<uint8_t>(*info.requiredDeviceType)) {
            return;
        } 

        _gpu = device;
        _extensions = info.requiredExtensions;
        // _extensions.reserve(deviceExtensions.size());
        // for (const auto & ext : deviceExtensions) {
        //     char * a = new char[255];
        //     std::strcpy(a, ext.extensionName);
        //     _extensions.emplace_back(a);
        // }

        vkGetPhysicalDeviceMemoryProperties(_gpu, &_memoryProperties);
    }

    bool PhysicalDevice::checkSurfaceSupport(const rhi::runtime::Surface & surface) noexcept {
        return checkSurfaceSupport(static_cast<const Surface &>(surface));
    }

    bool PhysicalDevice::checkSurfaceSupport(const Surface & surface) noexcept {
        auto vkSurface = surface.getVkSurface();

        uint32_t num = 0u;
        bool result = false;
        for (auto & family : _queues) {
            if (family.supportFlags(rhi::queue_flags::graphics)) {
                VkBool32 supportPresent = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(_gpu, num, vkSurface, &supportPresent);
                family.flags |= rhi::queue_flags::present;
                result |= supportPresent;
            }
        }
 
		return result;
    }

    PhysicalDevice::DepthFormatSupport PhysicalDevice::isDepthFormatSupported(VkFormat format, bool samplingSupport) const noexcept {
        VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(_gpu, format, &formatProperties);

        DepthFormatSupport support = DepthFormatSupport::eUnsupport;

        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) || !samplingSupport) { 
                support = static_cast<DepthFormatSupport>(static_cast<uint8_t>(support) | static_cast<uint8_t>(DepthFormatSupport::eOptimalTiling));
            }
		}

        if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if ((formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) || !samplingSupport) { 
                support = static_cast<DepthFormatSupport>(static_cast<uint8_t>(support) | static_cast<uint8_t>(DepthFormatSupport::elinearTiling));
            }
		}

        return support;
    }

    rhi::Format PhysicalDevice::getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport) const noexcept {
        static constexpr stl::array depthFormats = { VK_FORMAT_D16_UNORM, 
                                                     VK_FORMAT_D16_UNORM_S8_UINT, 
                                                     VK_FORMAT_X8_D24_UNORM_PACK32, 
                                                     VK_FORMAT_D24_UNORM_S8_UINT, 
                                                     VK_FORMAT_D32_SFLOAT, 
                                                     VK_FORMAT_D32_SFLOAT_S8_UINT 
                                                    };

		static constexpr stl::array depthStencilFormats = { VK_FORMAT_D16_UNORM_S8_UINT, 
                                                            VK_FORMAT_D24_UNORM_S8_UINT, 
                                                            VK_FORMAT_D32_SFLOAT_S8_UINT 
                                                        };

        uint8_t i = 0u;
        switch (desiredBpp) {
		    case 16u:
				break;
			case 24u:
				i = withStencil ? 1u : 3u;
				break;
			case 32u: 
				i = withStencil ? 2u : 4u;
				break;
			default:
				return rhi::Format::eUndefined;
		}

		const auto formats = withStencil ? 
			stl::span<const VkFormat>{depthStencilFormats.begin() + i, depthStencilFormats.size() - i} :
			stl::span<const VkFormat>{depthFormats.begin() + i,  depthFormats.size() - i};

        auto result = VkFormat::VK_FORMAT_UNDEFINED;
        for (const auto f : formats) {
            const auto support = isDepthFormatSupported(f, samplingSupport);
            if (static_cast<uint8_t>(support) & static_cast<uint8_t>(DepthFormatSupport::eOptimalTiling)) {
                result = f;
                break;
            } else if (result == VkFormat::VK_FORMAT_UNDEFINED && support != DepthFormatSupport::eUnsupport) {
                result = f;
            }
        }

        return static_cast<rhi::Format>(result);
    }

    rhi::FormatFeatureAccess PhysicalDevice::isFormatSupported(VkFormat format, VkFormatFeatureFlags featureFlags) const noexcept {
        VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(_gpu, format, &formatProperties);

        rhi::FormatFeatureAccess access{};

        if ((formatProperties.bufferFeatures & featureFlags) == featureFlags) {
            access.setBit(rhi::FormatFeatureAccess::Bit::eBufferFeaturesBit, true);
        }

        if ((formatProperties.linearTilingFeatures & featureFlags) == featureFlags) {
            access.setBit(rhi::FormatFeatureAccess::Bit::eLinearTilingFeaturesBit, true);
        }

        if ((formatProperties.optimalTilingFeatures & featureFlags) == featureFlags) {
            access.setBit(rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit, true);
        }
        
        return access;
    }

    uint32_t PhysicalDevice::getMemoryType(uint32_t requiredTypeBits, rhi::MemoryPropertyFlags properties) const noexcept {
        const auto asIntegerProperties = std::to_underlying(properties);
		for (uint32_t i = 0u; i < _memoryProperties.memoryTypeCount; ++i) {
			if (requiredTypeBits & (1 << i)) {
				if ((_memoryProperties.memoryTypes[i].propertyFlags & asIntegerProperties) == asIntegerProperties) {
					return i;
				}
			}
		}

		return 0xff'ff'ff'ffu;
	}

    stl::pair<uint32_t, const rhi::QueueFamily*> PhysicalDevice::getQueueFamily(uint8_t flags, uint32_t start) const noexcept {
        for (uint32_t i = start; i < _queues.size(); ++i) {
            if (_queues[i].supportFlags(flags)) {
                return {i, &_queues[i]};
            }
        }

        return {0xff'ff'ff'ffu, nullptr};
    }

    constexpr double kNanoToMilli = 1.0 / 1'000'000.0;
    float PhysicalDevice::timestampToMilliseconds(double time) const noexcept {
        // The results we got back do not actually contain a time value, 
        // but rather a number of "ticks". So to get the actual time value we need to translate these values first.
        // This is done using timestampPeriod limit of the physical device. 
        // It contains the number of nanoseconds it takes for a timestamp query value to be increased by 1 ("tick").
        // We want to display the delta between two timestamps in milliseconds, 
        // so in addition to the above rule we also multiply the value accordingly.
        return static_cast<float>(time * kNanoToMilli * _properties.limits.timestampPeriod);
    }

}