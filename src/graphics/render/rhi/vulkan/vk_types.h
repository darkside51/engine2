#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

namespace engine::vulkan {
namespace utils {

inline VkExtent2D convert2vk(const rhi::Extent2D & extent) noexcept {
    return {.width = extent.width, .height = extent.height};
}

inline VkExtent3D convert2vk(const rhi::Extent3D & extent) noexcept {
    return {.width = extent.width, .height = extent.height, .depth = extent.depth};
}

inline VkPhysicalDeviceFeatures convert2vk(const rhi::PhysicalDeviceFeatures & features) noexcept {
    VkPhysicalDeviceFeatures result = {};
    std::memset(&result, 0, sizeof(VkPhysicalDeviceFeatures));

    const uint32_t featuresCount = sizeof(rhi::PhysicalDeviceFeatures) / sizeof(bool);
	for (uint8_t i = 0u; i < featuresCount; ++i) {
        if (const bool src = *(reinterpret_cast<const bool*>(&(features)) + i); src) {
            VkBool32 & dst = *(reinterpret_cast<VkBool32*>(&(result)) + i);
            dst = 1u;
        }
    }

    return result;
}

inline VkColorSpaceKHR convert2vk(rhi::ColorSpace colorSpace) noexcept {
    switch (colorSpace) {
        case rhi::ColorSpace::eSRGB_Nonlinear:
            return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        case rhi::ColorSpace::eDISPLAY_P3_Nonlinear:
            return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
        case rhi::ColorSpace::eEXTENDED_SRGB_linear:
            return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
        case rhi::ColorSpace::eDISPLAY_P3_linear:
            return VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT;
        case rhi::ColorSpace::eDCI_P3_Nonlinear:
            return VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT;
        case rhi::ColorSpace::eBT709_linear:
            return VK_COLOR_SPACE_BT709_LINEAR_EXT;
        case rhi::ColorSpace::eBT709_Nonlinear:
            return VK_COLOR_SPACE_BT709_NONLINEAR_EXT;
        case rhi::ColorSpace::eBT2020_linear:
            return VK_COLOR_SPACE_BT2020_LINEAR_EXT;
        case rhi::ColorSpace::eHDR10_ST2084:
            return VK_COLOR_SPACE_HDR10_ST2084_EXT;
        case rhi::ColorSpace::eDOLBYVISION:
            return VK_COLOR_SPACE_DOLBYVISION_EXT;
        case rhi::ColorSpace::eHDR10_HLG:
            return VK_COLOR_SPACE_HDR10_HLG_EXT;
        case rhi::ColorSpace::eADOBERGB_linear:
            return VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT;
        case rhi::ColorSpace::eADOBERGB_Nonlinear:
            return VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
        case rhi::ColorSpace::ePASS_THROUGH:
            return VK_COLOR_SPACE_PASS_THROUGH_EXT;
        case rhi::ColorSpace::eEXTENDED_SRGB_Nonlinear:
            return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
        case rhi::ColorSpace::eDCI_P3_linear:
            return VK_COLOR_SPACE_DCI_P3_LINEAR_EXT;
    }
}

inline VkPresentModeKHR convert2vk(rhi::PresentMode mode) noexcept {
    switch (mode) {
        case rhi::PresentMode::eFifo:
            return VK_PRESENT_MODE_FIFO_KHR;
        case rhi::PresentMode::eImmidiate:
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case rhi::PresentMode::eMailbox:
            return VK_PRESENT_MODE_MAILBOX_KHR;
    }
}

inline VkImageType convert2vk(rhi::ImageType type) noexcept {
    switch (type) {
        case rhi::ImageType::eType_1D:
            return VK_IMAGE_TYPE_1D;
        case rhi::ImageType::eType_2D:
            return VK_IMAGE_TYPE_2D;
        case rhi::ImageType::eType_3D:
            return VK_IMAGE_TYPE_3D;
    }
}

inline VkImageViewType convert2vk(rhi::ImageViewType type) noexcept {
    switch (type) {
        case rhi::ImageViewType::eType_1D:
            return VK_IMAGE_VIEW_TYPE_1D;
        case rhi::ImageViewType::eType_2D:
            return VK_IMAGE_VIEW_TYPE_2D;
        case rhi::ImageViewType::eType_3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        case rhi::ImageViewType::eType_Cube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case rhi::ImageViewType::eType_1D_Array:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case rhi::ImageViewType::eType_2D_Array:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case rhi::ImageViewType::eType_Cube_Array:
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        case rhi::ImageViewType::eType_Undefined:
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

inline VkImageAspectFlags convert2vk(rhi::ImageAspectFlags flags) noexcept {
    VkImageAspectFlags result = VK_IMAGE_ASPECT_NONE;

    if (std::to_underlying(flags & rhi::ImageAspectFlags::eColor)) {
        result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageAspectFlags::eDepth)) {
        result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageAspectFlags::eStencil)) {
        result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageAspectFlags::eMetadata)) {
        result |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_METADATA_BIT;
    }

    return result;
}

inline VkImageUsageFlags convert2vk(rhi::ImageUsageFlags flags) noexcept {
    VkImageUsageFlags result = 0;

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eColor_Attachment)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eDepth_Stencil_Attachment)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eInput_Attachment)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eSampled)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eStorage)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eTransfer_Dst)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eTransfer_Src)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (std::to_underlying(flags & rhi::ImageUsageFlags::eTransient_Attachment)) {
        result |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    }

    return result;
}

inline VkSharingMode convert2vk(rhi::SharingMode mode) noexcept {
    switch (mode) {
        case rhi::SharingMode::eExclusive:
            return VK_SHARING_MODE_EXCLUSIVE;
        case rhi::SharingMode::eConcurrent:
            return VK_SHARING_MODE_CONCURRENT;
    }
}

inline VkSampleCountFlagBits convert2vk(uint8_t bits) noexcept {
    // constexpr auto isPow2 = [](int8_t a) noexcept { return !(a & (a - 1u));};
    // if (!isPow2(bits))
    switch (bits) {
        case 1: return VK_SAMPLE_COUNT_1_BIT;
        case 2: return VK_SAMPLE_COUNT_2_BIT;
        case 4: return VK_SAMPLE_COUNT_4_BIT;
        case 8: return VK_SAMPLE_COUNT_8_BIT;
        case 16: return VK_SAMPLE_COUNT_16_BIT;
        case 32: return VK_SAMPLE_COUNT_32_BIT;
        case 64: return VK_SAMPLE_COUNT_64_BIT;
    }
}

inline VkImageTiling convert2vk(rhi::ImageTiling tiling) noexcept {
    switch (tiling) {
        case rhi::ImageTiling::eOptimal:
            return VK_IMAGE_TILING_OPTIMAL;
        case rhi::ImageTiling::eLinear:
            return VK_IMAGE_TILING_LINEAR;
    }
}

inline VkImageCreateFlags convert2vk(rhi::ImageCreateFlags flags) noexcept {
    VkImageCreateFlags result = 0;
    if (std::to_underlying(flags & rhi::ImageCreateFlags::eMutable_Format)) {
        result |= VkImageCreateFlagBits::VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }
    if (std::to_underlying(flags & rhi::ImageCreateFlags::eCube_Compatible)) {
        result |= VkImageCreateFlagBits::VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    return result;
}

inline VkFormat convert2vk(rhi::Format format) noexcept {
    switch (format) {
        case rhi::Format::eUndefined: return VK_FORMAT_UNDEFINED;
        case rhi::Format::eR4G4_Unorm_Pack8: return VK_FORMAT_R4G4_UNORM_PACK8;
        case rhi::Format::eR4G4B4A4_Unorm_Pack16: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        case rhi::Format::eB4G4R4A4_Unorm_Pack16: return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        case rhi::Format::eR5G6B5_Unorm_Pack16: return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case rhi::Format::eB5G6R5_Unorm_Pack16: return VK_FORMAT_B5G6R5_UNORM_PACK16;
        case rhi::Format::eR5G5B5A1_Unorm_Pack16: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        case rhi::Format::eB5G5R5A1_Unorm_Pack16: return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        case rhi::Format::eA1R5G5B5_Unorm_Pack16: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;

        case rhi::Format::eR8_Unorm: return VK_FORMAT_R8_UNORM;
        case rhi::Format::eR8_Snorm: return VK_FORMAT_R8_SNORM;
        case rhi::Format::eR8_Uscaled: return VK_FORMAT_R8_USCALED;
        case rhi::Format::eR8_Sscaled: return VK_FORMAT_R8_SSCALED;
        case rhi::Format::eR8_Uint: return VK_FORMAT_R8_UINT;
        case rhi::Format::eR8_Sint: return VK_FORMAT_R8_SINT;
        case rhi::Format::eR8_Srgb: return VK_FORMAT_R8_SRGB;
        case rhi::Format::eR8G8_Unorm: return VK_FORMAT_R8G8_UNORM;
        case rhi::Format::eR8G8_Snorm: return VK_FORMAT_R8G8_SNORM;
        case rhi::Format::eR8G8_Uscaled: return VK_FORMAT_R8G8_USCALED;
        case rhi::Format::eR8G8_Sscaled: return VK_FORMAT_R8G8_SSCALED;
        case rhi::Format::eR8G8_Uint: return VK_FORMAT_R8G8_UINT;
        case rhi::Format::eR8G8_Sint: return VK_FORMAT_R8G8_SINT;
        case rhi::Format::eR8G8_Srgb: return VK_FORMAT_R8G8_SRGB;
        case rhi::Format::eR8G8B8_Unorm: return VK_FORMAT_R8G8B8_UNORM;
        case rhi::Format::eR8G8B8_Snorm: return VK_FORMAT_R8G8B8_SNORM;
        case rhi::Format::eR8G8B8_Uscaled: return VK_FORMAT_R8G8B8_USCALED;
        case rhi::Format::eR8G8B8_Sscaled: return VK_FORMAT_R8G8B8_SSCALED;
        case rhi::Format::eR8G8B8_Uint: return VK_FORMAT_R8G8B8_UINT;
        case rhi::Format::eR8G8B8_Sint: return VK_FORMAT_R8G8B8_SINT;
        case rhi::Format::eR8G8B8_Srgb: return VK_FORMAT_R8G8B8_SRGB;
        case rhi::Format::eB8G8R8_Unorm: return VK_FORMAT_B8G8R8_UNORM;
        case rhi::Format::eB8G8R8_Snorm: return VK_FORMAT_B8G8R8_SNORM;
        case rhi::Format::eB8G8R8_Uscaled: return VK_FORMAT_B8G8R8_USCALED;
        case rhi::Format::eB8G8R8_Sscaled: return VK_FORMAT_B8G8R8_SSCALED;
        case rhi::Format::eB8G8R8_Uint: return VK_FORMAT_B8G8R8_UINT;
        case rhi::Format::eB8G8R8_Sint: return VK_FORMAT_B8G8R8_SINT;
        case rhi::Format::eB8G8R8_Srgb: return VK_FORMAT_B8G8R8_SRGB;
        case rhi::Format::eR8G8B8A8_Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
        case rhi::Format::eR8G8B8A8_Snorm: return VK_FORMAT_R8G8B8A8_SNORM;
        case rhi::Format::eR8G8B8A8_Uscaled: return VK_FORMAT_R8G8B8A8_USCALED;
        case rhi::Format::eR8G8B8A8_Sscaled: return VK_FORMAT_R8G8B8A8_SSCALED;
        case rhi::Format::eR8G8B8A8_Uint: return VK_FORMAT_R8G8B8A8_UINT;
        case rhi::Format::eR8G8B8A8_Sint: return VK_FORMAT_R8G8B8A8_SINT;
        case rhi::Format::eR8G8B8A8_Srgb: return VK_FORMAT_R8G8B8A8_SRGB;
        case rhi::Format::eB8G8R8A8_Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
        case rhi::Format::eB8G8R8A8_Snorm: return VK_FORMAT_B8G8R8A8_SNORM;
        case rhi::Format::eB8G8R8A8_Uscaled: return VK_FORMAT_B8G8R8A8_USCALED;
        case rhi::Format::eB8G8R8A8_Sscaled: return VK_FORMAT_B8G8R8A8_SSCALED;
        case rhi::Format::eB8G8R8A8_Uint: return VK_FORMAT_B8G8R8A8_UINT;
        case rhi::Format::eB8G8R8A8_Sint: return VK_FORMAT_B8G8R8A8_SINT;
        case rhi::Format::eB8G8R8A8_Srgb: return VK_FORMAT_B8G8R8A8_SRGB;
        case rhi::Format::eA8B8G8R8_Unorm_Pack32: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
        case rhi::Format::eA8B8G8R8_Snorm_Pack32: return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
        case rhi::Format::eA8B8G8R8_Uscaled_Pack32: return VK_FORMAT_A8B8G8R8_USCALED_PACK32;
        case rhi::Format::eA8B8G8R8_Sscaled_Pack32: return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
        case rhi::Format::eA8B8G8R8_Uint_Pack32: return VK_FORMAT_A8B8G8R8_UINT_PACK32;
        case rhi::Format::eA8B8G8R8_Sint_Pack32: return VK_FORMAT_A8B8G8R8_SINT_PACK32;
        case rhi::Format::eA8B8G8R8_Srgb_Pack32: return VK_FORMAT_A8B8G8R8_SRGB_PACK32;

        case rhi::Format::eA2R10G10B10_Unorm_Pack32: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case rhi::Format::eA2R10G10B10_Snorm_Pack32: return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
        case rhi::Format::eA2R10G10B10_Uscaled_Pack32: return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
        case rhi::Format::eA2R10G10B10_Sscaled_Pack32: return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
        case rhi::Format::eA2R10G10B10_Uint_Pack32: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case rhi::Format::eA2R10G10B10_Sint_Pack32: return VK_FORMAT_A2R10G10B10_SINT_PACK32;
        case rhi::Format::eA2B10G10R10_Unorm_Pack32: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case rhi::Format::eA2B10G10R10_Snorm_Pack32: return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
        case rhi::Format::eA2B10G10R10_Uscaled_Pack32: return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
        case rhi::Format::eA2B10G10R10_Sscaled_Pack32: return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
        case rhi::Format::eA2B10G10R10_Uint_Pack32: return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case rhi::Format::eA2B10G10R10_Sint_Pack32: return VK_FORMAT_A2B10G10R10_SINT_PACK32;

        case rhi::Format::eR16_Unorm: return VK_FORMAT_R16_UNORM;
        case rhi::Format::eR16_Snorm: return VK_FORMAT_R16_SNORM;
        case rhi::Format::eR16_Uscaled: return VK_FORMAT_R16_USCALED;
        case rhi::Format::eR16_Sscaled: return VK_FORMAT_R16_SSCALED;
        case rhi::Format::eR16_Uint: return VK_FORMAT_R16_UINT;
        case rhi::Format::eR16_Sint: return VK_FORMAT_R16_SINT;
        case rhi::Format::eR16_Sfloat: return VK_FORMAT_R16_SFLOAT;
        case rhi::Format::eR16G16_Unorm: return VK_FORMAT_R16G16_UNORM;
        case rhi::Format::eR16G16_Snorm: return VK_FORMAT_R16G16_SNORM;
        case rhi::Format::eR16G16_Uscaled: return VK_FORMAT_R16G16_USCALED;
        case rhi::Format::eR16G16_Sscaled: return VK_FORMAT_R16G16_SSCALED;
        case rhi::Format::eR16G16_Uint: return VK_FORMAT_R16G16_UINT;
        case rhi::Format::eR16G16_Sint: return VK_FORMAT_R16G16_SINT;
        case rhi::Format::eR16G16_Sfloat: return VK_FORMAT_R16G16_SFLOAT;
        case rhi::Format::eR16G16B16_Unorm: return VK_FORMAT_R16G16B16_UNORM;
        case rhi::Format::eR16G16B16_Snorm: return VK_FORMAT_R16G16B16_SNORM;
        case rhi::Format::eR16G16B16_Uscaled: return VK_FORMAT_R16G16B16_USCALED;
        case rhi::Format::eR16G16B16_Sscaled: return VK_FORMAT_R16G16B16_SSCALED;
        case rhi::Format::eR16G16B16_Uint: return VK_FORMAT_R16G16B16_UINT;
        case rhi::Format::eR16G16B16_Sint: return VK_FORMAT_R16G16B16_SINT;
        case rhi::Format::eR16G16B16_Sfloat: return VK_FORMAT_R16G16B16_SFLOAT;
        case rhi::Format::eR16G16B16A16_Unorm: return VK_FORMAT_R16G16B16A16_UNORM;
        case rhi::Format::eR16G16B16A16_Snorm: return VK_FORMAT_R16G16B16A16_SNORM;
        case rhi::Format::eR16G16B16A16_Uscaled: return VK_FORMAT_R16G16B16A16_USCALED;
        case rhi::Format::eR16G16B16A16_Sscaled: return VK_FORMAT_R16G16B16A16_SSCALED;
        case rhi::Format::eR16G16B16A16_Uint: return VK_FORMAT_R16G16B16A16_UINT;
        case rhi::Format::eR16G16B16A16_Sint: return VK_FORMAT_R16G16B16A16_SINT;
        case rhi::Format::eR16G16B16A16_Sfloat: return VK_FORMAT_R16G16B16A16_SFLOAT;

        case rhi::Format::eR32_Uint: return VK_FORMAT_R32_UINT;
        case rhi::Format::eR32_Sint: return VK_FORMAT_R32_SINT;
        case rhi::Format::eR32_Sfloat: return VK_FORMAT_R32_SFLOAT;
        case rhi::Format::eR32G32_Uint: return VK_FORMAT_R32G32_UINT;
        case rhi::Format::eR32G32_Sint: return VK_FORMAT_R32G32_SINT;
        case rhi::Format::eR32G32_Sfloat: return VK_FORMAT_R32G32_SFLOAT;
        case rhi::Format::eR32G32B32_Uint: return VK_FORMAT_R32G32B32_UINT;
        case rhi::Format::eR32G32B32_Sint: return VK_FORMAT_R32G32B32_SINT;
        case rhi::Format::eR32G32B32_Sfloat: return VK_FORMAT_R32G32B32_SFLOAT;
        case rhi::Format::eR32G32B32A32_Uint: return VK_FORMAT_R32G32B32A32_UINT;
        case rhi::Format::eR32G32B32A32_Sint: return VK_FORMAT_R32G32B32A32_SINT;
        case rhi::Format::eR32G32B32A32_Sfloat: return VK_FORMAT_R32G32B32A32_SFLOAT;

        case rhi::Format::eR64_Uint: return VK_FORMAT_R64_UINT;
        case rhi::Format::eR64_Sint : return VK_FORMAT_R64_SINT;
        case rhi::Format::eR64_Sfloat: return VK_FORMAT_R64_SFLOAT;
        case rhi::Format::eR64G64_Uint: return VK_FORMAT_R64G64_UINT;
        case rhi::Format::eR64G64_Sint: return VK_FORMAT_R64G64_SINT;
        case rhi::Format::eR64G64_Sfloat: return VK_FORMAT_R64G64_SFLOAT;
        case rhi::Format::eR64G64B64_Uint: return VK_FORMAT_R64G64B64_UINT;
        case rhi::Format::eR64G64B64_Sint: return VK_FORMAT_R64G64B64_SINT;
        case rhi::Format::eR64G64B64_Sfloat: return VK_FORMAT_R64G64B64_SFLOAT;
        case rhi::Format::eR64G64B64A64_Uint: return VK_FORMAT_R64G64B64A64_UINT;
        case rhi::Format::eR64G64B64A64_Sint: return VK_FORMAT_R64G64B64A64_SINT;
        case rhi::Format::eR64G64B64A64_Sfloat: return VK_FORMAT_R64G64B64A64_SFLOAT;

        case rhi::Format::eB10G11R11_Ufloat_Pack32: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case rhi::Format::eE5B9G9R9_Ufloat_Pack32: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        case rhi::Format::eD16_Unorm: return VK_FORMAT_D16_UNORM;
        case rhi::Format::eX8_D24_Unorm_Pack32: return VK_FORMAT_X8_D24_UNORM_PACK32;
        case rhi::Format::eD32_Sfloat: return VK_FORMAT_D32_SFLOAT;
        case rhi::Format::eS8_Uint: return VK_FORMAT_S8_UINT;
        case rhi::Format::eD16_Unorm_S8_Uint: return VK_FORMAT_D16_UNORM_S8_UINT;
        case rhi::Format::eD24_Unorm_S8_Uint: return VK_FORMAT_D24_UNORM_S8_UINT;
        case rhi::Format::eD32_Sfloat_S8_Uint: return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case rhi::Format::eBC1_RGB_Unorm_Block: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case rhi::Format::eBC1_RGB_Srgb_Block: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        case rhi::Format::eBC1_RGBA_Unorm_Block: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case rhi::Format::eBC1_RGBA_Srgb_Block: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case rhi::Format::eBC2_Unorm_Block: return VK_FORMAT_BC2_UNORM_BLOCK;
        case rhi::Format::eBC2_Srgb_Block: return VK_FORMAT_BC2_SRGB_BLOCK;
        case rhi::Format::eBC3_Unorm_Block: return VK_FORMAT_BC3_UNORM_BLOCK;
        case rhi::Format::eBC3_Srgb_Block: return VK_FORMAT_BC3_SRGB_BLOCK;
        case rhi::Format::eBC4_Unorm_Block: return VK_FORMAT_BC4_UNORM_BLOCK;
        case rhi::Format::eBC4_Snorm_Block: return VK_FORMAT_BC4_SNORM_BLOCK;
        case rhi::Format::eBC5_Unorm_Block: return VK_FORMAT_BC5_UNORM_BLOCK;
        case rhi::Format::eBC5_Snorm_Block: return VK_FORMAT_BC5_SNORM_BLOCK;
        case rhi::Format::eBC6H_Ufloat_Block: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case rhi::Format::eBC6H_Sfloat_Block: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case rhi::Format::eBC7_Unorm_Block: return VK_FORMAT_BC7_UNORM_BLOCK;
        case rhi::Format::eBC7_Srgb_Block: return VK_FORMAT_BC7_SRGB_BLOCK;

        case rhi::Format::eETC2_R8G8B8_Unorm_Block: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case rhi::Format::eETC2_R8G8B8_Srgb_Block: return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        case rhi::Format::eETC2_R8G8B8A1_Unorm_Block: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case rhi::Format::eETC2_R8G8B8A1_Srgb_Block: return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        case rhi::Format::eETC2_R8G8B8A8_Unorm_Block: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case rhi::Format::eETC2_R8G8B8A8_Srgb_Block: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;

        case rhi::Format::eEAC_R11_Unorm_Block: return VK_FORMAT_EAC_R11_UNORM_BLOCK;
        case rhi::Format::eEAC_R11_Snorm_Block: return VK_FORMAT_EAC_R11_SNORM_BLOCK;
        case rhi::Format::eEAC_R11G11_Unorm_Block: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        case rhi::Format::eEAC_R11G11_Snorm_Block: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;

        case rhi::Format::eASTC_4x4_Unorm_Block: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case rhi::Format::eASTC_4x4_Srgb_Block: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case rhi::Format::eASTC_5x4_Unorm_Block: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        case rhi::Format::eASTC_5x4_Srgb_Block: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        case rhi::Format::eASTC_5x5_Unorm_Block: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case rhi::Format::eASTC_5x5_Srgb_Block: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        case rhi::Format::eASTC_6x5_Unorm_Block: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        case rhi::Format::eASTC_6x5_Srgb_Block: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        case rhi::Format::eASTC_6x6_Unorm_Block: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case rhi::Format::eASTC_6x6_Srgb_Block: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        case rhi::Format::eASTC_8x5_Unorm_Block: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        case rhi::Format::eASTC_8x5_Srgb_Block: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        case rhi::Format::eASTC_8x6_Unorm_Block: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        case rhi::Format::eASTC_8x6_Srgb_Block: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        case rhi::Format::eASTC_8x8_Unorm_Block: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case rhi::Format::eASTC_8x8_Srgb_Block: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case rhi::Format::eASTC_10x5_Unorm_Block: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        case rhi::Format::eASTC_10x5_Srgb_Block: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        case rhi::Format::eASTC_10x6_Unorm_Block: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        case rhi::Format::eASTC_10x6_Srgb_Block: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        case rhi::Format::eASTC_10x8_Unorm_Block: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        case rhi::Format::eASTC_10x8_Srgb_Block: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        case rhi::Format::eASTC_10x10_Unorm_Block: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case rhi::Format::eASTC_10x10_Srgb_Block: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        case rhi::Format::eASTC_12x10_Unorm_Block: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        case rhi::Format::eASTC_12x10_Srgb_Block: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        case rhi::Format::eASTC_12x12_Unorm_Block: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case rhi::Format::eASTC_12x12_Srgb_Block: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    }
}

inline rhi::Format convert2rhi(VkFormat format) noexcept {
    switch (format) {
        case VK_FORMAT_UNDEFINED: return rhi::Format::eUndefined;
        case VK_FORMAT_R4G4_UNORM_PACK8: return rhi::Format::eR4G4_Unorm_Pack8;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return rhi::Format::eR4G4B4A4_Unorm_Pack16;
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return rhi::Format::eB4G4R4A4_Unorm_Pack16;
        case VK_FORMAT_R5G6B5_UNORM_PACK16: return rhi::Format::eR5G6B5_Unorm_Pack16;
        case VK_FORMAT_B5G6R5_UNORM_PACK16: return rhi::Format::eB5G6R5_Unorm_Pack16;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return rhi::Format::eR5G5B5A1_Unorm_Pack16;
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return rhi::Format::eB5G5R5A1_Unorm_Pack16;
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return rhi::Format::eA1R5G5B5_Unorm_Pack16;

        case VK_FORMAT_R8_UNORM: return rhi::Format::eR8_Unorm;
        case VK_FORMAT_R8_SNORM: return rhi::Format::eR8_Snorm;
        case VK_FORMAT_R8_USCALED: return rhi::Format::eR8_Uscaled;
        case VK_FORMAT_R8_SSCALED: return rhi::Format::eR8_Sscaled;
        case VK_FORMAT_R8_UINT: return rhi::Format::eR8_Uint;
        case VK_FORMAT_R8_SINT: return rhi::Format::eR8_Sint;
        case VK_FORMAT_R8_SRGB: return rhi::Format::eR8_Srgb;
        case VK_FORMAT_R8G8_UNORM: return rhi::Format::eR8G8_Unorm;
        case VK_FORMAT_R8G8_SNORM: return rhi::Format::eR8G8_Snorm;
        case VK_FORMAT_R8G8_USCALED: return rhi::Format::eR8G8_Uscaled;
        case VK_FORMAT_R8G8_SSCALED: return rhi::Format::eR8G8_Sscaled;
        case VK_FORMAT_R8G8_UINT: return rhi::Format::eR8G8_Uint;
        case VK_FORMAT_R8G8_SINT: return rhi::Format::eR8G8_Sint;
        case VK_FORMAT_R8G8_SRGB: return rhi::Format::eR8G8_Srgb;
        case VK_FORMAT_R8G8B8_UNORM: return rhi::Format::eR8G8B8_Unorm;
        case VK_FORMAT_R8G8B8_SNORM: return rhi::Format::eR8G8B8_Snorm;
        case VK_FORMAT_R8G8B8_USCALED: return rhi::Format::eR8G8B8_Uscaled;
        case VK_FORMAT_R8G8B8_SSCALED: return rhi::Format::eR8G8B8_Sscaled;
        case VK_FORMAT_R8G8B8_UINT: return rhi::Format::eR8G8B8_Uint;
        case VK_FORMAT_R8G8B8_SINT: return rhi::Format::eR8G8B8_Sint;
        case VK_FORMAT_R8G8B8_SRGB: return rhi::Format::eR8G8B8_Srgb;
        case VK_FORMAT_B8G8R8_UNORM: return rhi::Format::eB8G8R8_Unorm;
        case VK_FORMAT_B8G8R8_SNORM: return rhi::Format::eB8G8R8_Snorm;
        case VK_FORMAT_B8G8R8_USCALED: return rhi::Format::eB8G8R8_Uscaled;
        case VK_FORMAT_B8G8R8_SSCALED: return rhi::Format::eB8G8R8_Sscaled;
        case VK_FORMAT_B8G8R8_UINT: return rhi::Format::eB8G8R8_Uint;
        case VK_FORMAT_B8G8R8_SINT: return rhi::Format::eB8G8R8_Sint;
        case VK_FORMAT_B8G8R8_SRGB: return rhi::Format::eB8G8R8_Srgb;
        case VK_FORMAT_R8G8B8A8_UNORM: return rhi::Format::eR8G8B8A8_Unorm;
        case VK_FORMAT_R8G8B8A8_SNORM: return rhi::Format::eR8G8B8A8_Snorm;
        case VK_FORMAT_R8G8B8A8_USCALED: return rhi::Format::eR8G8B8A8_Uscaled;
        case VK_FORMAT_R8G8B8A8_SSCALED: return rhi::Format::eR8G8B8A8_Sscaled;
        case VK_FORMAT_R8G8B8A8_UINT: return rhi::Format::eR8G8B8A8_Uint;
        case VK_FORMAT_R8G8B8A8_SINT: return rhi::Format::eR8G8B8A8_Sint;
        case VK_FORMAT_R8G8B8A8_SRGB: return rhi::Format::eR8G8B8A8_Srgb;
        case VK_FORMAT_B8G8R8A8_UNORM: return rhi::Format::eB8G8R8A8_Unorm;
        case VK_FORMAT_B8G8R8A8_SNORM: return rhi::Format::eB8G8R8A8_Snorm;
        case VK_FORMAT_B8G8R8A8_USCALED: return rhi::Format::eB8G8R8A8_Uscaled;
        case VK_FORMAT_B8G8R8A8_SSCALED: return rhi::Format::eB8G8R8A8_Sscaled;
        case VK_FORMAT_B8G8R8A8_UINT: return rhi::Format::eB8G8R8A8_Uint;
        case VK_FORMAT_B8G8R8A8_SINT: return rhi::Format::eB8G8R8A8_Sint;
        case VK_FORMAT_B8G8R8A8_SRGB: return rhi::Format::eB8G8R8A8_Srgb;
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return rhi::Format::eA8B8G8R8_Unorm_Pack32;
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return rhi::Format::eA8B8G8R8_Snorm_Pack32;
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return rhi::Format::eA8B8G8R8_Uscaled_Pack32;
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return rhi::Format::eA8B8G8R8_Sscaled_Pack32;
        case VK_FORMAT_A8B8G8R8_UINT_PACK32: return rhi::Format::eA8B8G8R8_Uint_Pack32;
        case VK_FORMAT_A8B8G8R8_SINT_PACK32: return rhi::Format::eA8B8G8R8_Sint_Pack32;
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return rhi::Format::eA8B8G8R8_Srgb_Pack32;

        case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return rhi::Format::eA2R10G10B10_Unorm_Pack32;
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return rhi::Format::eA2R10G10B10_Snorm_Pack32;
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return rhi::Format::eA2R10G10B10_Uscaled_Pack32;
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return rhi::Format::eA2R10G10B10_Sscaled_Pack32;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32: return rhi::Format::eA2R10G10B10_Uint_Pack32;
        case VK_FORMAT_A2R10G10B10_SINT_PACK32: return rhi::Format::eA2R10G10B10_Sint_Pack32;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return rhi::Format::eA2B10G10R10_Unorm_Pack32;
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return rhi::Format::eA2B10G10R10_Snorm_Pack32;
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return rhi::Format::eA2B10G10R10_Uscaled_Pack32;
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return rhi::Format::eA2B10G10R10_Sscaled_Pack32;
        case VK_FORMAT_A2B10G10R10_UINT_PACK32: return rhi::Format::eA2B10G10R10_Uint_Pack32;
        case VK_FORMAT_A2B10G10R10_SINT_PACK32: return rhi::Format::eA2B10G10R10_Sint_Pack32;

        case VK_FORMAT_R16_UNORM: return rhi::Format::eR16_Unorm;
        case VK_FORMAT_R16_SNORM: return rhi::Format::eR16_Snorm;
        case VK_FORMAT_R16_USCALED: return rhi::Format::eR16_Uscaled;
        case VK_FORMAT_R16_SSCALED: return rhi::Format::eR16_Sscaled;
        case VK_FORMAT_R16_UINT: return rhi::Format::eR16_Uint;
        case VK_FORMAT_R16_SINT: return rhi::Format::eR16_Sint;
        case VK_FORMAT_R16_SFLOAT: return rhi::Format::eR16_Sfloat;
        case VK_FORMAT_R16G16_UNORM: return rhi::Format::eR16G16_Unorm;
        case VK_FORMAT_R16G16_SNORM: return rhi::Format::eR16G16_Snorm;
        case VK_FORMAT_R16G16_USCALED: return rhi::Format::eR16G16_Uscaled;
        case VK_FORMAT_R16G16_SSCALED: return rhi::Format::eR16G16_Sscaled;
        case VK_FORMAT_R16G16_UINT: return rhi::Format::eR16G16_Uint;
        case VK_FORMAT_R16G16_SINT: return rhi::Format::eR16G16_Sint;
        case VK_FORMAT_R16G16_SFLOAT: return rhi::Format::eR16G16_Sfloat;
        case VK_FORMAT_R16G16B16_UNORM: return rhi::Format::eR16G16B16_Unorm;
        case VK_FORMAT_R16G16B16_SNORM: return rhi::Format::eR16G16B16_Snorm;
        case VK_FORMAT_R16G16B16_USCALED: return rhi::Format::eR16G16B16_Uscaled;
        case VK_FORMAT_R16G16B16_SSCALED: return rhi::Format::eR16G16B16_Sscaled;
        case VK_FORMAT_R16G16B16_UINT: return rhi::Format::eR16G16B16_Uint;
        case VK_FORMAT_R16G16B16_SINT: return rhi::Format::eR16G16B16_Sint;
        case VK_FORMAT_R16G16B16_SFLOAT: return rhi::Format::eR16G16B16_Sfloat;
        case VK_FORMAT_R16G16B16A16_UNORM: return rhi::Format::eR16G16B16A16_Unorm;
        case VK_FORMAT_R16G16B16A16_SNORM: return rhi::Format::eR16G16B16A16_Snorm;
        case VK_FORMAT_R16G16B16A16_USCALED: return rhi::Format::eR16G16B16A16_Uscaled;
        case VK_FORMAT_R16G16B16A16_SSCALED: return rhi::Format::eR16G16B16A16_Sscaled;
        case VK_FORMAT_R16G16B16A16_UINT: return rhi::Format::eR16G16B16A16_Uint;
        case VK_FORMAT_R16G16B16A16_SINT: return rhi::Format::eR16G16B16A16_Sint;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return rhi::Format::eR16G16B16A16_Sfloat;

        case VK_FORMAT_R32_UINT: return rhi::Format::eR32_Uint;
        case VK_FORMAT_R32_SINT: return rhi::Format::eR32_Sint;
        case VK_FORMAT_R32_SFLOAT: return rhi::Format::eR32_Sfloat;
        case VK_FORMAT_R32G32_UINT: return rhi::Format::eR32G32_Uint;
        case VK_FORMAT_R32G32_SINT: return rhi::Format::eR32G32_Sint;
        case VK_FORMAT_R32G32_SFLOAT: return rhi::Format::eR32G32_Sfloat;
        case VK_FORMAT_R32G32B32_UINT: return rhi::Format::eR32G32B32_Uint;
        case VK_FORMAT_R32G32B32_SINT: return rhi::Format::eR32G32B32_Sint;
        case VK_FORMAT_R32G32B32_SFLOAT: return rhi::Format::eR32G32B32_Sfloat;
        case VK_FORMAT_R32G32B32A32_UINT: return rhi::Format::eR32G32B32A32_Uint;
        case VK_FORMAT_R32G32B32A32_SINT: return rhi::Format::eR32G32B32A32_Sint;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return rhi::Format::eR32G32B32A32_Sfloat;

        case VK_FORMAT_R64_UINT: return rhi::Format::eR64_Uint;
        case VK_FORMAT_R64_SINT: return rhi::Format::eR64_Sint;
        case VK_FORMAT_R64_SFLOAT: return rhi::Format::eR64_Sfloat;
        case VK_FORMAT_R64G64_UINT: return rhi::Format::eR64G64_Uint;
        case VK_FORMAT_R64G64_SINT: return rhi::Format::eR64G64_Sint;
        case VK_FORMAT_R64G64_SFLOAT: return rhi::Format::eR64G64_Sfloat;
        case VK_FORMAT_R64G64B64_UINT: return rhi::Format::eR64G64B64_Uint;
        case VK_FORMAT_R64G64B64_SINT: return rhi::Format::eR64G64B64_Sint;
        case VK_FORMAT_R64G64B64_SFLOAT: return rhi::Format::eR64G64B64_Sfloat;
        case VK_FORMAT_R64G64B64A64_UINT: return rhi::Format::eR64G64B64A64_Uint;
        case VK_FORMAT_R64G64B64A64_SINT: return rhi::Format::eR64G64B64A64_Sint;
        case VK_FORMAT_R64G64B64A64_SFLOAT: return rhi::Format::eR64G64B64A64_Sfloat;

        case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return rhi::Format::eB10G11R11_Ufloat_Pack32;
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return rhi::Format::eE5B9G9R9_Ufloat_Pack32;
        case VK_FORMAT_D16_UNORM: return rhi::Format::eD16_Unorm;
        case VK_FORMAT_X8_D24_UNORM_PACK32: return rhi::Format::eX8_D24_Unorm_Pack32;
        case VK_FORMAT_D32_SFLOAT: return rhi::Format::eD32_Sfloat;
        case VK_FORMAT_S8_UINT: return rhi::Format::eS8_Uint;
        case VK_FORMAT_D16_UNORM_S8_UINT: return rhi::Format::eD16_Unorm_S8_Uint;
        case VK_FORMAT_D24_UNORM_S8_UINT: return rhi::Format::eD24_Unorm_S8_Uint;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return rhi::Format::eD32_Sfloat_S8_Uint;

        case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return rhi::Format::eBC1_RGB_Unorm_Block;
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return rhi::Format::eBC1_RGB_Srgb_Block;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return rhi::Format::eBC1_RGBA_Unorm_Block;
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return rhi::Format::eBC1_RGBA_Srgb_Block;
        case VK_FORMAT_BC2_UNORM_BLOCK: return rhi::Format::eBC2_Unorm_Block;
        case VK_FORMAT_BC2_SRGB_BLOCK: return rhi::Format::eBC2_Srgb_Block;
        case VK_FORMAT_BC3_UNORM_BLOCK: return rhi::Format::eBC3_Unorm_Block;
        case VK_FORMAT_BC3_SRGB_BLOCK: return rhi::Format::eBC3_Srgb_Block;
        case VK_FORMAT_BC4_UNORM_BLOCK: return rhi::Format::eBC4_Unorm_Block;
        case VK_FORMAT_BC4_SNORM_BLOCK: return rhi::Format::eBC4_Snorm_Block;
        case VK_FORMAT_BC5_UNORM_BLOCK: return rhi::Format::eBC5_Unorm_Block;
        case VK_FORMAT_BC5_SNORM_BLOCK: return rhi::Format::eBC5_Snorm_Block;
        case VK_FORMAT_BC6H_UFLOAT_BLOCK: return rhi::Format::eBC6H_Ufloat_Block;
        case VK_FORMAT_BC6H_SFLOAT_BLOCK: return rhi::Format::eBC6H_Sfloat_Block;
        case VK_FORMAT_BC7_UNORM_BLOCK: return rhi::Format::eBC7_Unorm_Block;
        case VK_FORMAT_BC7_SRGB_BLOCK: return rhi::Format::eBC7_Srgb_Block;

        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return rhi::Format::eETC2_R8G8B8_Unorm_Block;
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return rhi::Format::eETC2_R8G8B8_Srgb_Block;
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return rhi::Format::eETC2_R8G8B8A1_Unorm_Block;
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return rhi::Format::eETC2_R8G8B8A1_Srgb_Block;
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return rhi::Format::eETC2_R8G8B8A8_Unorm_Block;
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return rhi::Format::eETC2_R8G8B8A8_Srgb_Block;

        case VK_FORMAT_EAC_R11_UNORM_BLOCK: return rhi::Format::eEAC_R11_Unorm_Block;
        case VK_FORMAT_EAC_R11_SNORM_BLOCK: return rhi::Format::eEAC_R11_Snorm_Block;
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return rhi::Format::eEAC_R11G11_Unorm_Block;
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return rhi::Format::eEAC_R11G11_Snorm_Block;

        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return rhi::Format::eASTC_4x4_Unorm_Block;
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return rhi::Format::eASTC_4x4_Srgb_Block;
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return rhi::Format::eASTC_5x4_Unorm_Block;
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return rhi::Format::eASTC_5x4_Srgb_Block;
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return rhi::Format::eASTC_5x5_Unorm_Block;
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return rhi::Format::eASTC_5x5_Srgb_Block;
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return rhi::Format::eASTC_6x5_Unorm_Block;
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return rhi::Format::eASTC_6x5_Srgb_Block;
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return rhi::Format::eASTC_6x6_Unorm_Block;
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return rhi::Format::eASTC_6x6_Srgb_Block;
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return rhi::Format::eASTC_8x5_Unorm_Block;
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return rhi::Format::eASTC_8x5_Srgb_Block;
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return rhi::Format::eASTC_8x6_Unorm_Block;
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return rhi::Format::eASTC_8x6_Srgb_Block;
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return rhi::Format::eASTC_8x8_Unorm_Block;
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return rhi::Format::eASTC_8x8_Srgb_Block;
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return rhi::Format::eASTC_10x5_Unorm_Block;
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return rhi::Format::eASTC_10x5_Srgb_Block;
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return rhi::Format::eASTC_10x6_Unorm_Block;
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return rhi::Format::eASTC_10x6_Srgb_Block;
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return rhi::Format::eASTC_10x8_Unorm_Block;
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return rhi::Format::eASTC_10x8_Srgb_Block;
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return rhi::Format::eASTC_10x10_Unorm_Block;
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return rhi::Format::eASTC_10x10_Srgb_Block;
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return rhi::Format::eASTC_12x10_Unorm_Block;
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return rhi::Format::eASTC_12x10_Srgb_Block;
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return rhi::Format::eASTC_12x12_Unorm_Block;
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return rhi::Format::eASTC_12x12_Srgb_Block;
    }

    assert(false); // unknown format
    return rhi::Format::eUndefined;
}

inline VkResolveModeFlags convert2vk(rhi::ResolveModeFlags flags) noexcept {
    VkResolveModeFlags result = VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE;

    if (std::to_underlying(flags & rhi::ResolveModeFlags::eSample_Zero)) {
        result |= VkResolveModeFlagBits::VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    }

    if (std::to_underlying(flags & rhi::ResolveModeFlags::eAverage)) {
        result |= VkResolveModeFlagBits::VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    if (std::to_underlying(flags & rhi::ResolveModeFlags::eMin)) {
        result |= VkResolveModeFlagBits::VK_RESOLVE_MODE_MIN_BIT;
    }

    if (std::to_underlying(flags & rhi::ResolveModeFlags::eMax)) {
        result |= VkResolveModeFlagBits::VK_RESOLVE_MODE_MAX_BIT;
    }

    return result;
}

inline VkAttachmentLoadOp convert2vk(rhi::AttachmentLoadOp op) noexcept {
    switch (op) {
        case rhi::AttachmentLoadOp::eLoad: return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
        case rhi::AttachmentLoadOp::eClear: return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        case rhi::AttachmentLoadOp::eDont_Care: return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case rhi::AttachmentLoadOp::eNone: return VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_NONE_EXT;
    }
}

inline VkAttachmentStoreOp convert2vk(rhi::AttachmentStoreOp op) noexcept {
    switch (op) {
        case rhi::AttachmentStoreOp::eStore: return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        case rhi::AttachmentStoreOp::eDont_Care: return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case rhi::AttachmentStoreOp::eNone: return VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_NONE_KHR;
    }
}

inline VkImageLayout convert2vk(rhi::ImageLayout layout) noexcept {
    switch (layout) {
        case rhi::ImageLayout::eUndefined: VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        case rhi::ImageLayout::eGeneral: VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        case rhi::ImageLayout::eColor_Attachment_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Stencil_Read_Only_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eShader_Read_Only_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eTransfer_Src_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case rhi::ImageLayout::eTransfer_Dst_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case rhi::ImageLayout::ePreinitialized : VkImageLayout::VK_IMAGE_LAYOUT_PREINITIALIZED;
        case rhi::ImageLayout::eDepth_Read_Only_Stencil_Attachment_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Attachment_Stencil_Read_Only_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eDepth_Attachment_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Read_Only_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eStencil_Attachment_Optimal : VkImageLayout::VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eStencil_Read_Only_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eRead_Only_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eAttachment_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::ePresent_Src: VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case rhi::ImageLayout::eShared_Present: VkImageLayout::VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
        case rhi::ImageLayout::eFragment_Density_Map_Optimal_Ext: VkImageLayout::VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
        case rhi::ImageLayout::eFragment_Shading_Rate_Attachment_Optimal: VkImageLayout::VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        case rhi::ImageLayout::eAttachment_Feedback_Loop_Optimal_Ext: VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT;
    }
}

inline VkPipelineStageFlags convert2vk(rhi::PipelineStageFlags flags) noexcept {
    return static_cast<VkPipelineStageFlags>(flags);
}

inline VkAccessFlags convert2vk(rhi::AccessFlags flags) noexcept {
    return static_cast<VkAccessFlags>(flags);
}

inline VkDependencyFlags convert2vk(rhi::DependencyFlags flags) noexcept {
    return static_cast<VkDependencyFlags>(flags);
}

}

}