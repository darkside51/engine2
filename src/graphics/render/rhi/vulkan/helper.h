#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace engine::vulkan::helper {

    inline void getVulkanInstanceLayers(std::vector<VkLayerProperties>& layers) noexcept {
		uint32_t layerCount = 0u;
		const auto result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		if (result != VK_SUCCESS) { return; }
		layers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
	}

	inline void getVulkanInstanceExtensions(std::vector<VkExtensionProperties>& extensions) noexcept {
		uint32_t extensionCount = 0u;
		const auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // has 160 x 3 = 480bytes memory leak here
		if (result != VK_SUCCESS) { return; }
		extensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	}

	inline void getVulkanPhysicalDevices(VkInstance instance, std::vector<VkPhysicalDevice> & devices) noexcept {
		uint32_t deviceCount = 0u;
        const auto result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (result != VK_SUCCESS) { return; }
        devices.resize(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	}

	inline void getVulkanPhysicalDeviceExtensionProperties(VkPhysicalDevice gpu, std::vector<VkExtensionProperties> & deviceExtProperties) noexcept {
		uint32_t deviceExtCount = 0u;
		const auto result = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtCount, nullptr);
		if (result != VK_SUCCESS) { return; }
		deviceExtProperties.resize(deviceExtCount);
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtCount, deviceExtProperties.data());
	}

	inline void getVulkanPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice gpu, std::vector<VkQueueFamilyProperties> & gpuQueueFamilyProperties) noexcept {
		uint32_t queueFamilyCount = 0u;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
		gpuQueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, gpuQueueFamilyProperties.data());
	}

	inline void getVulkanPhysicalDeviceSurfaceFormats(VkPhysicalDevice gpu, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> & surfaceFormats) noexcept {
		uint32_t formatCount = 0u;
		const auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);
		if (result != VK_SUCCESS) { return; }
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, surfaceFormats.data());
	}

	inline void getVulkanPhysicalDevicePresentModes(VkPhysicalDevice gpu, VkSurfaceKHR surface, std::vector<VkPresentModeKHR> & presentModes) noexcept {
		uint32_t presentModeCount = 0u;
        const auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);
		if (result != VK_SUCCESS) { return; }
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes.data());
	}

	inline void getVulkanSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage> & images) noexcept {
		uint32_t imagesCount = 0u;
        const auto result = vkGetSwapchainImagesKHR(device, swapchain, &imagesCount, nullptr);
		if (result != VK_SUCCESS) { return; }
        images.resize(imagesCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imagesCount, images.data());
	}
}