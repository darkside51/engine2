#include "swapchain.h"

#include "instance.h"
#include "device.h"
#include "physical_device.h"
#include "surface.h"
#include "image.h"
#include "helper.h"
#include "vk_types.h"

#include "log/log.h"

#include <algorithm>
#include <cassert>

namespace engine::vulkan {
    
    Swapchain::Swapchain(const Device & device, const Surface & surface, const rhi::SwapchainCreateInfo & info) : 
    _device(device),
    _surface(surface),
    _info(info) {
        LOG_I("vulkan: swapchain created");
    }

    Swapchain::~Swapchain() {
        destroy(_swapchain);
        LOG_I("vulkan: swapchain destroyed");
    }

    uint32_t Swapchain::getImagesCount() const noexcept { return _images.size(); }

    void Swapchain::resize(uint32_t w, uint32_t h, bool vsync) {
        const auto surfaceSupportDetails = getSurfaceSupportDetails(_device.gpu(), _surface);

        const auto & capabilities = surfaceSupportDetails.capabilities;
        const auto & minExtent = capabilities.minImageExtent;
        const auto & maxExtent = capabilities.maxImageExtent;

        const uint32_t width = std::clamp(w, minExtent.width, maxExtent.width);
		const uint32_t height = std::clamp(h, minExtent.height, maxExtent.height);
		const VkExtent2D swapchainExtent = {width , height};

        // determine the number of images
		uint32_t imagesCount = std::max(_info.imagesCount, capabilities.minImageCount);
		if (capabilities.maxImageCount > 0u) {
            imagesCount = std::min(_info.imagesCount, capabilities.maxImageCount);
		}

        // find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			// prefer a non-rotated transform
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		} else {
			preTransform = capabilities.currentTransform;
		}

        // find surface format
        VkSurfaceFormatKHR surfaceFormat;
        // if the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ((surfaceSupportDetails.formats.size() == 1) && (surfaceSupportDetails.formats[0].format == VK_FORMAT_UNDEFINED)) {
			surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
			surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // or surfaceSupportDetails.formats[0].colorSpace ??;
		} else {
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
            uint8_t priority = 0u;
			for (const auto & format : surfaceSupportDetails.formats) {
                if (format.colorSpace == utils::convert2vk(_info.colorSpace)) {
                    surfaceFormat.format = format.format;
					surfaceFormat.colorSpace = format.colorSpace;
                    break;
                } else if (format.format == VK_FORMAT_B8G8R8A8_UNORM && priority < 1u) {
					surfaceFormat.format = format.format;
					surfaceFormat.colorSpace = format.colorSpace;
					priority = 1u;
				} else if (format.format == VkFormat::VK_FORMAT_UNDEFINED && priority == 0u) {
					surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
					surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
				}
			}
		}

        _format = utils::convert2rhi(surfaceFormat.format);
        _colorSpace = utils::convert2rhi(surfaceFormat.colorSpace);

        // the VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
		// this mode waits for the vertical blank ("v-sync")
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        if (!vsync) {
            for (auto mode : surfaceSupportDetails.presentModes) {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					presentMode = mode;
					break;
                } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					presentMode = mode;
                }
            }
        }

        // find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// select the first composite alpha format available
		const std::array<VkCompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};

		for (auto& compositeAlphaFlag : compositeAlphaFlags) {
			if (capabilities.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

        // store the old swapchain
        VkSwapchainKHR oldSwapchain = _swapchain;

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = nullptr;
        swapchainCreateInfo.flags = 0;
        swapchainCreateInfo.surface = _surface();
        swapchainCreateInfo.minImageCount = imagesCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = swapchainExtent;
        swapchainCreateInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
        swapchainCreateInfo.imageArrayLayers = 1u;

        if (_info.queueFamilies.size() > 1u) {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        } else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        swapchainCreateInfo.queueFamilyIndexCount = _info.queueFamilies.size();
        swapchainCreateInfo.pQueueFamilyIndices = _info.queueFamilies.data();

        swapchainCreateInfo.presentMode = presentMode;
		// setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
		swapchainCreateInfo.oldSwapchain = oldSwapchain;
		// setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.compositeAlpha = compositeAlpha;

        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		// enable transfer source on swap chain images if supported
		if (_info.supportTransferSrc && capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		// enable transfer destination on swap chain images if supported
		if (_info.supportTransferDst && capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();

        if (auto result = vkCreateSwapchainKHR(_device(), &swapchainCreateInfo, allocationCallbacks, &_swapchain); 
            result != VkResult::VK_SUCCESS) {
			assert(false);
            LOG_E("vulkan: vkCreateSwapchainKHR result: %d", result);
            return;
		}

        // if an existing swap chain is re-created, destroy the old swap chain
		// this also cleans up all the presentable images
		if (oldSwapchain != VK_NULL_HANDLE) {
            destroy(oldSwapchain);
        }

        std::vector<VkImage> images;
        helper::getVulkanSwapchainImages(_device(), _swapchain, images);

        const auto realImagesCount = images.size();

        _images.reserve(realImagesCount);
        _imageViews.resize(realImagesCount);

        rhi::ImageInfo imageInfo;
        imageInfo.format = utils::convert2rhi(surfaceFormat.format);
        imageInfo.extent = {width, height, 1u};
        imageInfo.arrayLayers = 1u;
        imageInfo.mipLevels = 1u;
        imageInfo.type = rhi::ImageType::eType_2D;
        imageInfo.aspectFlags = rhi::ImageAspectFlags::eColor;
        imageInfo.queueFamilyIndices = _info.queueFamilies;
        imageInfo.usage = rhi::ImageUsageFlags::eColor_Attachment;
        for (uint32_t i = 0u; i < realImagesCount; ++i) {
            auto & image = _images.emplace_back(const_cast<Device&>(_device), imageInfo, images[i]);
            _imageViews[i] = image.getImageView(rhi::ImageViewType::eType_2D, 0u, 1u, 0, 1u, rhi::ImageAspectFlags::eColor);
        }

        // create semaphores
        _imageAvailableSemaphores.resize(images.size());
        VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		    .pNext = nullptr,
		    .flags = 0
        };
        for (auto & semaphore : _imageAvailableSemaphores) {
            vkCreateSemaphore(_device(), &semaphoreCreateInfo, allocationCallbacks, &semaphore);
        }

        VkFenceCreateInfo fenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };
        vkCreateFence(_device(), &fenceCreateInfo, allocationCallbacks, &_imageAvailableFence);

        LOG_I("vulkan: swapchain resized, image count = %d", realImagesCount);
    }

    void Swapchain::waitFenceAndReset(uint64_t timeout) noexcept {
        vkWaitForFences(_device(), 1, &_imageAvailableFence, VK_TRUE, timeout);
        vkResetFences(_device(), 1, &_imageAvailableFence);
    }

    std::pair<uint32_t, bool> Swapchain::acquireNextImage(uint32_t frameIndex) const noexcept {
        uint32_t imageIndex = 0u;
        VkResult result = vkAcquireNextImageKHR(_device(), _swapchain, UINT64_MAX, _imageAvailableSemaphores[frameIndex], _imageAvailableFence, &imageIndex);
        bool success = true;
        switch (result) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
            LOG_I("vulkan: acquireNextImage result VK_SUBOPTIMAL_KHR");
            break;
        default:
            success = false;
            LOG_E("vulkan: acquireNextImage result: %d", result);
            break;
        }

        return {imageIndex, success};
    }

    const Image & Swapchain::getImage(uint8_t index) const noexcept { return _images[index]; }
    Image & Swapchain::getImage(uint8_t index) noexcept { return _images[index]; }

    void Swapchain::destroy(VkSwapchainKHR swapchain) {
        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();

        if (_imageAvailableFence != VK_NULL_HANDLE) {
            vkResetFences(_device(), 1, &_imageAvailableFence);
        }
        vkDestroyFence(_device(), _imageAvailableFence, allocationCallbacks);

        for (auto & s: _imageAvailableSemaphores) {
            vkDestroySemaphore(_device(), s, allocationCallbacks);
        }
        _imageAvailableSemaphores.clear();

        _images.clear();
        _imageViews.clear();

        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(_device(), swapchain, allocationCallbacks);
            swapchain = VK_NULL_HANDLE;
        }
    }

}