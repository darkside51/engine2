#pragma once

#include "graphics/render/rhi/common/device.h"
#include "graphics/render/rhi/common/types.h"

#include "utils/dynamic_array.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

namespace engine::vulkan {
namespace utils {

inline VkOffset2D convert2vk(const rhi::Offset2D & offset) noexcept {
    return {.x = offset.x, .y = offset.y};
}

inline VkOffset3D convert2vk(const rhi::Offset3D & offset) noexcept {
    return {.x = offset.x, .y = offset.y, .z = offset.z};
}

inline VkExtent2D convert2vk(const rhi::Extent2D & extent) noexcept {
    return {.width = extent.width, .height = extent.height};
}

inline VkExtent3D convert2vk(const rhi::Extent3D & extent) noexcept {
    return {.width = extent.width, .height = extent.height, .depth = extent.depth};
}

inline VkClearValue convert2vk(const rhi::ClearValue & clearValue) noexcept {
    return {clearValue.color.value[0u], 
            clearValue.color.value[1u], 
            clearValue.color.value[2u], 
            clearValue.color.value[3u]};
}

template<size_t N>
inline auto convert2vk(const std::array<rhi::ClearValue, N> & array) -> std::array<VkClearValue, N> {
    std::array<VkClearValue, N> result;
    for (size_t i = 0u; i < N; ++i) {
        result[i] = convert2vk(array[i]);
    }
    return result;
}

inline VkBufferCopy convert2vk(const rhi::CopyInfo & info) noexcept {
    return VkBufferCopy {
        .srcOffset = info.srcOffset,
        .dstOffset = info.dstOffset,
        .size = info.size
    };
}

inline VkColorComponentFlags convert2vk(rhi::ColorComponent c) noexcept {
    VkColorComponentFlags result = 0;
    if (std::to_underlying(c & rhi::ColorComponent::eR)) {
        result |= VK_COLOR_COMPONENT_R_BIT;
    }
    if (std::to_underlying(c & rhi::ColorComponent::eG)) {
        result |= VK_COLOR_COMPONENT_G_BIT;
    }
    if (std::to_underlying(c & rhi::ColorComponent::eB)) {
        result |= VK_COLOR_COMPONENT_B_BIT;
    }
    if (std::to_underlying(c & rhi::ColorComponent::eA)) {
        result |= VK_COLOR_COMPONENT_A_BIT;
    }
    return result;
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

    assert(false);
    return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}

inline rhi::ColorSpace convert2rhi(VkColorSpaceKHR colorSpace) noexcept {
    switch (colorSpace) {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return rhi::ColorSpace::eSRGB_Nonlinear;
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
            return rhi::ColorSpace::eDISPLAY_P3_Nonlinear;
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            return rhi::ColorSpace::eEXTENDED_SRGB_linear;
        case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
            return rhi::ColorSpace::eDISPLAY_P3_linear;
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
            return rhi::ColorSpace::eDCI_P3_Nonlinear;
        case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            return rhi::ColorSpace::eBT709_linear;
        case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
            return rhi::ColorSpace::eBT709_Nonlinear;
        case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            return rhi::ColorSpace::eBT2020_linear;
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            return rhi::ColorSpace::eHDR10_ST2084;
        case VK_COLOR_SPACE_DOLBYVISION_EXT:
            return rhi::ColorSpace::eDOLBYVISION;
        case VK_COLOR_SPACE_HDR10_HLG_EXT:
            return rhi::ColorSpace::eHDR10_HLG;
        case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
            return rhi::ColorSpace::eADOBERGB_linear;
        case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
            return rhi::ColorSpace::eADOBERGB_Nonlinear;
        case VK_COLOR_SPACE_PASS_THROUGH_EXT:
            return rhi::ColorSpace::ePASS_THROUGH;
        case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
            return rhi::ColorSpace::eEXTENDED_SRGB_Nonlinear;
        default: return rhi::ColorSpace::eSRGB_Nonlinear;
    }

    assert(false);
    return rhi::ColorSpace::eSRGB_Nonlinear;
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

inline VkMemoryPropertyFlags convert2vk(rhi::MemoryPropertyFlags flags) noexcept {
    VkMemoryPropertyFlags result = 0u;

    if (std::to_underlying(flags & rhi::MemoryPropertyFlags::eDeviceLocal)) {
        result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    if (std::to_underlying(flags & rhi::MemoryPropertyFlags::eHostVisible)) {
        result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    if (std::to_underlying(flags & rhi::MemoryPropertyFlags::eHostCoherent)) {
        result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    if (std::to_underlying(flags & rhi::MemoryPropertyFlags::eHostCached)) {
        result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }

    if (std::to_underlying(flags & rhi::MemoryPropertyFlags::eLazilyAllocated)) {
        result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    if (std::to_underlying(flags & rhi::MemoryPropertyFlags::eProtected)) {
        result |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_PROTECTED_BIT;
    }

    return result;
}

inline VkBufferUsageFlags convert2vk(rhi::BufferUsageFlags flags) noexcept {
    VkBufferUsageFlags result = 0;

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eTransfer_Src)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eTransfer_Dst)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eUniform_Texel_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eStorage_Texel_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eUniform_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eStorage_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eIndex_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eVertex_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eIndirect_Buffer)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eShader_Device_Address)) {
        // result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eVideo_Decode_Src_Khr)) {
        // result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eTransform_Feedback_Buffer_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eTransform_Feedback_Counter_Buffer_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eConditional_Rendering_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eAcceleration_Structure_Build_Input_Read_Only_Khr)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eAcceleration_Structure_Storage_Khr)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eShader_Binding_Table_Khr)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eVideo_Encode_Dst_Khr)) {
        // result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eVideo_Encode_Src_Khr)) {
        // result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eSampler_Descriptor_Buffer_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eResource_Descriptor_Buffer_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::ePush_Descriptors_Descriptor_Buffer_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eMicromap_Build_Input_Read_Only_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eMicromap_Storage_Ext)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::BufferUsageFlags::eRay_Tracing_NV)) {
        result |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
    }

    return result;
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
    return VK_SHARING_MODE_EXCLUSIVE;
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
    return VK_SAMPLE_COUNT_1_BIT;
}

inline VkImageTiling convert2vk(rhi::ImageTiling tiling) noexcept {
    switch (tiling) {
        case rhi::ImageTiling::eOptimal:
            return VK_IMAGE_TILING_OPTIMAL;
        case rhi::ImageTiling::eLinear:
            return VK_IMAGE_TILING_LINEAR;
    }
    return VK_IMAGE_TILING_OPTIMAL;
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

inline VkBufferCreateFlags convert2vk(rhi::BufferCreateFlags flags) noexcept {
    VkBufferCreateFlags result = 0;
    if (std::to_underlying(flags & rhi::BufferCreateFlags::eSparse_Binding)) {
        result |= VkBufferCreateFlagBits::VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
    }
    if (std::to_underlying(flags & rhi::BufferCreateFlags::eSparse_Residency)) {
        result |= VkBufferCreateFlagBits::VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
    }
    if (std::to_underlying(flags & rhi::BufferCreateFlags::eSparse_Aliased)) {
        result |= VkBufferCreateFlagBits::VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
    }
    if (std::to_underlying(flags & rhi::BufferCreateFlags::eProtected)) {
        result |= VkBufferCreateFlagBits::VK_BUFFER_CREATE_PROTECTED_BIT;
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

    assert(false);
    return VK_FORMAT_UNDEFINED;
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
        default: return rhi::Format::eUndefined;
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
        case rhi::ImageLayout::eUndefined: return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        case rhi::ImageLayout::eGeneral: return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
        case rhi::ImageLayout::eColor_Attachment_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Stencil_Read_Only_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eShader_Read_Only_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eTransfer_Src_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case rhi::ImageLayout::eTransfer_Dst_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case rhi::ImageLayout::ePreinitialized : return VkImageLayout::VK_IMAGE_LAYOUT_PREINITIALIZED;
        case rhi::ImageLayout::eDepth_Read_Only_Stencil_Attachment_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Attachment_Stencil_Read_Only_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eDepth_Attachment_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eDepth_Read_Only_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eStencil_Attachment_Optimal : return VkImageLayout::VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::eStencil_Read_Only_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eRead_Only_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        case rhi::ImageLayout::eAttachment_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        case rhi::ImageLayout::ePresent_Src: return VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case rhi::ImageLayout::eShared_Present: return VkImageLayout::VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
        case rhi::ImageLayout::eFragment_Density_Map_Optimal_Ext: return VkImageLayout::VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
        case rhi::ImageLayout::eFragment_Shading_Rate_Attachment_Optimal: return VkImageLayout::VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        case rhi::ImageLayout::eAttachment_Feedback_Loop_Optimal_Ext: return VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT;
    }
}

inline VkPipelineStageFlags convert2vk(rhi::PipelineStageFlags flags) noexcept {
    return static_cast<VkPipelineStageFlags>(flags);
}

template<size_t N>
inline auto convert2vk(const std::array<rhi::PipelineStageFlags, N> & array) -> std::array<VkPipelineStageFlags, N> {
    std::array<VkPipelineStageFlags, N> result;
    for (size_t i = 0u; i < N; ++i) {
        result[i] = convert2vk(array[i]);
    }
    return result;
}

inline VkAccessFlags convert2vk(rhi::AccessFlags flags) noexcept {
    return static_cast<VkAccessFlags>(flags);
}

inline VkDependencyFlags convert2vk(rhi::DependencyFlags flags) noexcept {
    return static_cast<VkDependencyFlags>(flags);
}

inline VkCommandPoolCreateFlags convert2vk(rhi::CommandPoolCreateFlags flags) noexcept {
    return static_cast<VkCommandPoolCreateFlagBits>(flags);
}

inline VkCommandBufferLevel convert2vk(rhi::CommandBufferLevel level) noexcept {
    return static_cast<VkCommandBufferLevel>(level);
}

inline VkSubpassContents convert2vk(rhi::SubpassContents contents) noexcept {
    switch (contents) {
        case rhi::SubpassContents::ePrimaryBufferInline:
            return VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE;
        case rhi::SubpassContents::eSecondaryBuffers:
            return VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
    }
    return VK_SUBPASS_CONTENTS_INLINE;
}

inline VkQueryPipelineStatisticFlags convert2vk(rhi::QueryPipelineStatisticFlags flags) noexcept {
    VkQueryPipelineStatisticFlags result = static_cast<VkQueryPipelineStatisticFlagBits>(0);

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eInputAssemblyVertices)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eInputAssemblyPrimitives)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eVertexShaderInvocations)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eGeometryShaderInvocations)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eGeometryShaderPrimitives)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eClippingInvocations)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eClippingPrimitives)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eFragmentShaderInvocations)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eTessellationControlShaderPatches)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eTessellationEvaluationShaderInvocations)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eComputeShaderInvocations)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eTaskShaderInvocationsEXT)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::QueryPipelineStatisticFlags::eMeshShaderInvocationsEXT)) {
        result |= VkQueryPipelineStatisticFlagBits::VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT;
    }

    return result;
}

inline VkQueryControlFlags convert2vk(rhi::QueryControlFlags flags) noexcept {
    VkQueryControlFlags result = static_cast<VkQueryControlFlags>(0);

    if (std::to_underlying(flags & rhi::QueryControlFlags::ePrecise)) {
        result |= VkQueryControlFlagBits::VK_QUERY_CONTROL_PRECISE_BIT;
    }

    return result;
}

inline VkFenceCreateFlags convert2vk(rhi::FenceCreateFlags flags) noexcept {
    switch (flags) {
        case rhi::FenceCreateFlags::eSignaled:
            return VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;
        default:
        return 0;
    }
}

inline VkDescriptorType convert2vk(rhi::shaders::DescriptorType type) noexcept {
    using namespace rhi::shaders;
    switch(type) {
        case DescriptorType::eSampler: return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::eCombined_Image_Sampler: return VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
        case DescriptorType::eSampled_Image: return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; 
        case DescriptorType::eStorage_Image: return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; 
        case DescriptorType::eUniform_Texel_Buffer: return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; 
        case DescriptorType::eStorage_Texel_Buffer: return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER; 
        case DescriptorType::eUniform_Buffer: return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
        case DescriptorType::eStorage_Buffer: return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 
        case DescriptorType::eUniform_Buffer_Dynamic: return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; 
        case DescriptorType::eStorage_Buffer_Dynamic: return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; 
        case DescriptorType::eInput_Attachment: return VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; 
        case DescriptorType::eInline_Uniform_Block: return VkDescriptorType::VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
        case DescriptorType::eAcceleretion_Structure: return VkDescriptorType::VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; 
        default: return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }

    return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

inline VkDescriptorPoolCreateFlags convert2vk(rhi::shaders::DescriptorPoolFlags flags) noexcept {
    VkDescriptorPoolCreateFlags result = static_cast<VkDescriptorPoolCreateFlags>(0);

    if (std::to_underlying(flags & rhi::shaders::DescriptorPoolFlags::eFree_Descriptor_Set)) {
        result |= VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    }

    if (std::to_underlying(flags & rhi::shaders::DescriptorPoolFlags::eUpdate_After_Bind)) {
        result |= VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    }

    return result;
}

inline VkDescriptorSetLayoutCreateFlags convert2vk(rhi::shaders::DescriptorSetLayoutFlags flags) noexcept {
    VkDescriptorSetLayoutCreateFlags result = static_cast<VkDescriptorSetLayoutCreateFlags>(0);

    if (std::to_underlying(flags & rhi::shaders::DescriptorSetLayoutFlags::eUpdate_After_Bind_Pool)) {
        result |= VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    }

    if (std::to_underlying(flags & rhi::shaders::DescriptorSetLayoutFlags::ePush_Descriptor)) {
        result |= VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    }

    if (std::to_underlying(flags & rhi::shaders::DescriptorSetLayoutFlags::eDescriptor_Buffer)) {
        result |= VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    if (std::to_underlying(flags & rhi::shaders::DescriptorSetLayoutFlags::eEmbedded_Immutable_Samplers)) {
        result |= VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_EMBEDDED_IMMUTABLE_SAMPLERS_BIT_EXT;
    }

    return result;
}

inline VkShaderStageFlags convert2vk(rhi::shaders::Stages stages) noexcept {
    VkShaderStageFlags result = static_cast<VkShaderStageFlags>(0);

    if (std::to_underlying(stages & rhi::shaders::Stages::eVertex)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eTesselation_Control)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eTesselation_Evaluation)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eGeometry)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eFragment)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eCompute)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eRaygen)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eAny_Hit)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eClosest_Hit)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eMiss)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eIntersection)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eCallable)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eTask)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_TASK_BIT_EXT;
    }

    if (std::to_underlying(stages & rhi::shaders::Stages::eMesh)) {
        result |= VkShaderStageFlagBits::VK_SHADER_STAGE_MESH_BIT_EXT;
    }
    return result;
}

inline VkIndexType convert2vk(rhi::IndexType type) noexcept {
    switch (type) {
        case rhi::IndexType::eU16: return VkIndexType::VK_INDEX_TYPE_UINT16;
        case rhi::IndexType::eU32: return VkIndexType::VK_INDEX_TYPE_UINT32;
        case rhi::IndexType::eU8: return VkIndexType::VK_INDEX_TYPE_UINT8_EXT;
    }
    return VkIndexType::VK_INDEX_TYPE_UINT16;
}

inline VkVertexInputRate convert2vk(rhi::VertexInputRate rate) noexcept {
    switch (rate) {
        case rhi::VertexInputRate::eVertex: return VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
        case rhi::VertexInputRate::eInstance: return VkVertexInputRate::VK_VERTEX_INPUT_RATE_INSTANCE;
    }
}

///////////////////////
inline VkPipelineBindPoint convert2vk(rhi::PipelineBindPoint bindPoint) noexcept {
    switch (bindPoint) {
        case rhi::PipelineBindPoint::eGraphics : return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        case rhi::PipelineBindPoint::eCompute : return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE;
        case rhi::PipelineBindPoint::eRay_Tracing : return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    }
    return VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
}

inline VkCompareOp convert2vk(rhi::CompareOp op) noexcept {
    switch (op) {
        case rhi::CompareOp::eNever: return VkCompareOp::VK_COMPARE_OP_NEVER;
        case rhi::CompareOp::eLess: return VkCompareOp::VK_COMPARE_OP_LESS;
        case rhi::CompareOp::eEqual: return VkCompareOp::VK_COMPARE_OP_EQUAL;
        case rhi::CompareOp::eLequal: return VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        case rhi::CompareOp::eGreater: return VkCompareOp::VK_COMPARE_OP_GREATER;
        case rhi::CompareOp::eNot_Equal: return VkCompareOp::VK_COMPARE_OP_NOT_EQUAL;
        case rhi::CompareOp::eGequal: return VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL;
        case rhi::CompareOp::eAlways: return VkCompareOp::VK_COMPARE_OP_ALWAYS;
    }
    return VkCompareOp::VK_COMPARE_OP_NEVER;
}

inline VkStencilOp convert2vk(rhi::StencilOp op) noexcept {
    switch (op) {
        case rhi::StencilOp::eKeep: return VkStencilOp::VK_STENCIL_OP_KEEP;
        case rhi::StencilOp::eZero: return VkStencilOp::VK_STENCIL_OP_ZERO;
        case rhi::StencilOp::eReplace: return VkStencilOp::VK_STENCIL_OP_REPLACE;
        case rhi::StencilOp::eInc_And_Clamp: return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case rhi::StencilOp::eDec_And_Clamp: return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case rhi::StencilOp::eInvert: return VkStencilOp::VK_STENCIL_OP_INVERT;
        case rhi::StencilOp::eInc_And_Wrap: return VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case rhi::StencilOp::eDec_And_Wrap: return VkStencilOp::VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }
    return VkStencilOp::VK_STENCIL_OP_KEEP;
}

inline VkBlendOp convert2vk(rhi::BlendOp op) noexcept {
    switch (op) {
        case rhi::BlendOp::eAdd: return VkBlendOp::VK_BLEND_OP_ADD;
        case rhi::BlendOp::eSubtract: return VkBlendOp::VK_BLEND_OP_SUBTRACT;
        case rhi::BlendOp::eReverse_Subtract: return VkBlendOp::VK_BLEND_OP_REVERSE_SUBTRACT;
        case rhi::BlendOp::eMin: return VkBlendOp::VK_BLEND_OP_MIN;
        case rhi::BlendOp::eMax: return VkBlendOp::VK_BLEND_OP_MAX;
    }
    return VkBlendOp::VK_BLEND_OP_ADD;
}

inline VkBlendFactor convert2vk(rhi::BlendFactor factor) noexcept {
    switch (factor) {
        case rhi::BlendFactor::eZero: return VkBlendFactor::VK_BLEND_FACTOR_ZERO;
        case rhi::BlendFactor::eOne: return VkBlendFactor::VK_BLEND_FACTOR_ONE;
        case rhi::BlendFactor::eSrc_Color: return VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;
        case rhi::BlendFactor::eOne_Minus_Src_Color: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case rhi::BlendFactor::eDst_Color: return VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;
        case rhi::BlendFactor::eOne_Minus_Dst_Color: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case rhi::BlendFactor::eSrc_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
        case rhi::BlendFactor::eOne_Minus_Src_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case rhi::BlendFactor::eDst_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;
        case rhi::BlendFactor::eOne_Minus_Dst_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case rhi::BlendFactor::eConst_Color: return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_COLOR;
        case rhi::BlendFactor::eOne_Minus_Const_Color: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case rhi::BlendFactor::eConst_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case rhi::BlendFactor::eOne_Minus_Const_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case rhi::BlendFactor::eSrc_Alpha_Saturate: return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        default: return VkBlendFactor::VK_BLEND_FACTOR_ZERO;
        // case rhi::BlendFactor::eSrc1_Color: return VkBlendFactor::VK_BLEND_FACTOR_SRC1_COLOR;
        // case rhi::BlendFactor::eOne_Minus_Src1_Color: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        // case rhi::BlendFactor::eSrc1_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_SRC1_ALPHA;
        // case rhi::BlendFactor::eOne_Minus_Src1_Alpha: return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    return VkBlendFactor::VK_BLEND_FACTOR_ZERO;
}

inline VkPrimitiveTopology convert2vk(rhi::PrimitiveTopology topology) noexcept {
    switch (topology) {
        case rhi::PrimitiveTopology::ePoints: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case rhi::PrimitiveTopology::eLines: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case rhi::PrimitiveTopology::eLine_Strip: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case rhi::PrimitiveTopology::eTriangles: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case rhi::PrimitiveTopology::eTriangle_Strip: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case rhi::PrimitiveTopology::eTriangle_Fan: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case rhi::PrimitiveTopology::eLines_With_Adjacency: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
        case rhi::PrimitiveTopology::eLine_Strip_With_Adjacency: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
        case rhi::PrimitiveTopology::eTriangles_With_Adjacency: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
        case rhi::PrimitiveTopology::eTriangle_Strip_With_Adjacency: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
        case rhi::PrimitiveTopology::ePatchs: return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    }
    return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

inline VkPolygonMode convert2vk(rhi::PolygonMode mode) noexcept {
    switch (mode) {
        case rhi::PolygonMode::eFill: return VkPolygonMode::VK_POLYGON_MODE_FILL;
        case rhi::PolygonMode::eLine: return VkPolygonMode::VK_POLYGON_MODE_LINE;
        case rhi::PolygonMode::ePoint: return VkPolygonMode::VK_POLYGON_MODE_POINT;
    }
    return VkPolygonMode::VK_POLYGON_MODE_FILL;
}

inline VkFrontFace convert2vk(rhi::FrontFace mode) noexcept {
    switch (mode) {
        case rhi::FrontFace::eCounter_Clockwise: return VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case rhi::FrontFace::eClockwise: return VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
    }
    return VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

inline VkCullModeFlags convert2vk(rhi::CullMode mode) noexcept {
    switch (mode) {
        case rhi::CullMode::eNone: return VkCullModeFlagBits::VK_CULL_MODE_NONE;
        case rhi::CullMode::eFront: return VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT;
        case rhi::CullMode::eBack: return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
        case rhi::CullMode::eFront_And_Back: return VkCullModeFlagBits::VK_CULL_MODE_FRONT_AND_BACK;
    }
    return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
}

inline VkStencilOpState convert2vk(rhi::StencilOpState state) noexcept {
    return {
        .failOp = convert2vk(state.failOp),
        .passOp = convert2vk(state.passOp),
        .depthFailOp = convert2vk(state.depthFailOp),
        .compareOp = convert2vk(state.compareOp),
        .compareMask = state.compareMask,
        .writeMask = state.writeMask,
        .reference = state.reference
    };
}

inline VkPipelineColorBlendAttachmentState convert2vk(rhi::BlendMode mode) noexcept {
    return {
        .blendEnable = mode.blendEnable,
        .srcColorBlendFactor = convert2vk(mode.srcColorFactor),
        .dstColorBlendFactor = convert2vk(mode.dstColorFactor),
        .colorBlendOp = convert2vk(mode.colorBlendOp),
        .srcAlphaBlendFactor = convert2vk(mode.srcAlphaFactor),
        .dstAlphaBlendFactor = convert2vk(mode.dstAlphaFactor),
        .alphaBlendOp = convert2vk(mode.alphaBlendOp),
        .colorWriteMask = convert2vk(mode.colorWriteMask),
    };
}

inline std::pair<std::array<VkDynamicState, rhi::kDynamicStatesCount>, uint8_t> convert2vk(rhi::DynamicState states) noexcept {
    std::pair<std::array<VkDynamicState, rhi::kDynamicStatesCount>, uint8_t> result = {};
    auto & [resultStates, count] = result;

    if (std::to_underlying(states & rhi::DynamicState::eViewport)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT;
    } 
    if (std::to_underlying(states & rhi::DynamicState::eScissor)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_SCISSOR;
    }
    if (std::to_underlying(states & rhi::DynamicState::eLine_Width)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_LINE_WIDTH;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Bias)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BIAS;
    }
    if (std::to_underlying(states & rhi::DynamicState::eBlend_Constants)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_BLEND_CONSTANTS;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Bounds)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BOUNDS;
    }
    if (std::to_underlying(states & rhi::DynamicState::eStencil_Compare_Mask)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
    }
    if (std::to_underlying(states & rhi::DynamicState::eStencil_Write_Mask)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
    }
    if (std::to_underlying(states & rhi::DynamicState::eStencil_Refference)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_STENCIL_REFERENCE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eCull_Mode)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_CULL_MODE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eFront_Face)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_FRONT_FACE;
    }
    if (std::to_underlying(states & rhi::DynamicState::ePrimitive_Topology)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY;
    }
    if (std::to_underlying(states & rhi::DynamicState::eViewport_With_Count)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT;
    }
    if (std::to_underlying(states & rhi::DynamicState::eScissor_With_Count)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT;
    }
    if (std::to_underlying(states & rhi::DynamicState::eVertex_Input_Binding_Stride)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Test_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Write_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Compare_Op)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_COMPARE_OP;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Bounds_Test_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eStencil_Test_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eStencil_Op)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_STENCIL_OP;
    }
    if (std::to_underlying(states & rhi::DynamicState::eRasterizer_Discard_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE;
    }
    if (std::to_underlying(states & rhi::DynamicState::eDepth_Bias_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE;
    }
    if (std::to_underlying(states & rhi::DynamicState::ePrimitive_Restart_Enable)) {
        resultStates[count++] = VkDynamicState::VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE;
    }
    return result;
}


inline VkVertexInputBindingDescription convert2vk(rhi::VertexInputBinding description) noexcept {
    return {
        .binding = description.binding,
        .stride = description.stride,
        .inputRate = convert2vk(description.inputRate)
    };
}

template<size_t N>
inline auto convert2vk(const std::array<rhi::VertexInputBinding, N> & array) noexcept -> std::array<VkVertexInputBindingDescription, N> {
    std::array<VkVertexInputBindingDescription, N> result;
    for (size_t i = 0u; i < N; ++i) {
        result[i] = convert2vk(array[i]);
    }
    return result;
}

inline auto convert2vk(std::span<const rhi::VertexInputBinding> array) {
    dynamic_array<VkVertexInputBindingDescription, 4u> result;
    result.reserve(array.size());
    auto view = result.view();
    for (size_t i = 0u; i < array.size(); ++i) {
        view[i] = convert2vk(array[i]);
    }
    return result;
}

inline VkVertexInputAttributeDescription convert2vk(rhi::VertexInputAttribute attribute) noexcept {
    return {
        .location = attribute.location,
        .binding = attribute.binding,
        .format = convert2vk(attribute.format),
        .offset = attribute.offset
    };
}

template<size_t N>
inline auto convert2vk(const std::array<rhi::VertexInputAttribute, N> & array) noexcept -> std::array<VkVertexInputAttributeDescription, N> {
    std::array<VkVertexInputAttributeDescription, N> result;
    for (size_t i = 0u; i < N; ++i) {
        result[i] = convert2vk(array[i]);
    }
    return result;
}

inline auto convert2vk(std::span<const rhi::VertexInputAttribute> array) {
    dynamic_array<VkVertexInputAttributeDescription, 8u> result;
    result.reserve(array.size());
    auto view = result.view();
    for (size_t i = 0u; i < array.size(); ++i) {
        view[i] = convert2vk(array[i]);
    }
    return result;
}


}

}