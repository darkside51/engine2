#include "instance.h"
#include "helper.h"

#include "utils/assert.h"
#include "log/log.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace engine::vulkan {

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

        switch (messageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: [[fallthrough]];
        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            LOG_I("vulkan: %s", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            LOG_E("vulkan error: %s", pCallbackData->pMessage);
            ENGINE_BREAK
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: 
            LOG_D("vulkan warning: %s", pCallbackData->pMessage);
            //ENGINE_BREAK
            break;
        default:
            break;
        }
        
        return VK_FALSE;
    }

    Instance::Instance(const rhi::InstanceCreateInfo & info) : _apiVersion(VK_MAKE_VERSION(info.apiVersion->major, info.apiVersion->minor, info.apiVersion->patch)) {
        _allocationCallbacks = static_cast<const VkAllocationCallbacks *>(info.allocationCallback);

        VkApplicationInfo applicationInfo;
		applicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName     = "";
        applicationInfo.pEngineName          = "engine";
		applicationInfo.applicationVersion   = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.engineVersion        = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.apiVersion           = _apiVersion;

        stl::vector<VkExtensionProperties> supportedExtensions;
        helper::getVulkanInstanceExtensions(supportedExtensions);

        stl::vector<VkLayerProperties> supportedLayers;
		helper::getVulkanInstanceLayers(supportedLayers);

        stl::vector<const char*> instanceExtensions;

        const auto addInstanceExtension = [&instanceExtensions, &supportedExtensions](const char* extName) -> bool {
			if (std::find_if(supportedExtensions.begin(), supportedExtensions.end(), [extName](const VkExtensionProperties& ext) { return std::strcmp(ext.extensionName, extName) == 0; }) != supportedExtensions.end()) {
				instanceExtensions.push_back(extName);
				return true;
			}
			return false;
		};

		const auto addInstanceLayer = [this, &supportedLayers](const char* layerName) {
			if (std::find_if(supportedLayers.begin(), supportedLayers.end(), [layerName](const VkLayerProperties& p) { return std::strcmp(p.layerName, layerName) == 0; }) != supportedLayers.end()) {
				_instanceLayers.push_back(layerName);
			}
		};

        for (auto && ext : info.extensions) {
			addInstanceExtension(ext.data());
		}

		for (auto && layer : info.layers) {
			addInstanceLayer(layer.data());
		}

        VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_instanceLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = _instanceLayers.data();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

        if (const auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance); result != VK_SUCCESS) {
            LOG_E("vkCreateInstance error: %d", result);
            _instance = VK_NULL_HANDLE;
            return;
        }

        LOG_I("vulkan: instance create successfull");

        // create debug messenger
        if (info.createDebugMessages) {
            VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;

            auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
            if (!createMessenger || createMessenger(_instance, &createInfo, _allocationCallbacks, &_debugMessenger) != VK_SUCCESS) {
                LOG_E("vkCreateDebugUtilsMessengerEXT error");
            }
        }
    }

    Instance::~Instance() {
        if (_instance) {
            if (_debugMessenger) {
                auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
                if (destroyMessenger) {
                    destroyMessenger(_instance, _debugMessenger, _allocationCallbacks);
                } else {
                    LOG_E("vkDestroyDebugUtilsMessengerEXT error");
                }
            }

            vkDestroyInstance(_instance, _allocationCallbacks);
            LOG_I("vulkan: instance destroyed");
        }
    }
    
} 
