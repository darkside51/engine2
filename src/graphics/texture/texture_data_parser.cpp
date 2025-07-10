#include "texture_data_parser.h"

#include <ktx.h>
#include <vk_funcs.h>
#include <ktxvulkan.h>
#include <texture.h>
#include <formatsize.h>
#include <sail-c++/sail-c++.h>

namespace engine::graphics {

    rhi::Format convert(VkFormat format) noexcept {
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

    rhi::Format convert(SailPixelFormat format) noexcept {
        switch (format) {
        // Grayscale formats.
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_GRAYSCALE: return rhi::Format::eR8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_GRAYSCALE: return rhi::Format::eR16_Unorm;

        // Grayscale with alpha formats.
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_GRAYSCALE_ALPHA: return rhi::Format::eR4G4_Unorm_Pack8;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_GRAYSCALE_ALPHA: return rhi::Format::eR8G8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_GRAYSCALE_ALPHA: return rhi::Format::eR16G16_Unorm;

        // Packed formats.
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGB555: return rhi::Format::eR5G5B5A1_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGR555: return rhi::Format::eB5G5R5A1_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGB565: return rhi::Format::eR5G6B5_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGR565: return rhi::Format::eB5G6R5_Unorm_Pack16;

        // RGB formats.
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_RGB: return rhi::Format::eR8G8B8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_BGR: return rhi::Format::eB8G8R8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP48_RGB: return rhi::Format::eR16G16B16_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP48_BGR: return rhi::Format::eUndefined; // !

        // RGBA/X formats. X = unused color channel.
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGBX: return rhi::Format::eR4G4B4A4_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGRX: return rhi::Format::eB4G4R4A4_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_XRGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_XBGR: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGBA: return rhi::Format::eR4G4B4A4_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGRA: return rhi::Format::eB4G4R4A4_Unorm_Pack16;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_ARGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_ABGR: return rhi::Format::eUndefined; // !

        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBX: return rhi::Format::eR8G8B8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRX: return rhi::Format::eB8G8R8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_XRGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_XBGR: return rhi::Format::eA8B8G8R8_Unorm_Pack32;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBA: return rhi::Format::eR8G8B8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRA: return rhi::Format::eB8G8R8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_ARGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_ABGR: return rhi::Format::eA8B8G8R8_Unorm_Pack32;

        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_RGBX: return rhi::Format::eR16G16B16A16_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_BGRX: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_XRGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_XBGR: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_RGBA: return rhi::Format::eR16G16B16A16_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_BGRA: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_ARGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_ABGR: return rhi::Format::eUndefined; // !
        
        default: return rhi::Format::eUndefined;
        }
        return rhi::Format::eUndefined;
    }

    SailPixelFormat convert(rhi::Format format) noexcept {
        switch (format) {
        // Grayscale formats.
        case rhi::Format::eR8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_GRAYSCALE;
        case rhi::Format::eR16_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_GRAYSCALE;

        // Grayscale with alpha formats.
        case rhi::Format::eR4G4_Unorm_Pack8: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_GRAYSCALE_ALPHA;
        case rhi::Format::eR8G8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_GRAYSCALE_ALPHA;
        case rhi::Format::eR16G16_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_GRAYSCALE_ALPHA;

        // Packed formats.
        case rhi::Format::eR5G5B5A1_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGB555;
        case rhi::Format::eB5G5R5A1_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGR555;
        case rhi::Format::eR5G6B5_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGB565;
        case rhi::Format::eB5G6R5_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGR565;

        // RGB formats.
        case rhi::Format::eR8G8B8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_RGB;
        case rhi::Format::eB8G8R8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_BGR;
        case rhi::Format::eR16G16B16_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP48_RGB;

        // RGBA/X formats. X = unused color channel.
        case rhi::Format::eR4G4B4A4_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGBA;
        case rhi::Format::eB4G4R4A4_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGRA;

        case rhi::Format::eR8G8B8A8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBA;
        case rhi::Format::eB8G8R8A8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRA;
        case rhi::Format::eA8B8G8R8_Unorm_Pack32: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_ABGR;

        case rhi::Format::eR16G16B16A16_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_RGBA;
        
        default: SailPixelFormat::SAIL_PIXEL_FORMAT_UNKNOWN;
        }
        return SailPixelFormat::SAIL_PIXEL_FORMAT_UNKNOWN;
    }

    TextureMetaInfo parseTextureMetaInfoOnly(std::span<const std::byte> bytes) {
        sail::image_input imageInput(bytes.data(), bytes.size());
        auto [image, _] = imageInput.probe();

        if (!image.is_valid()) {
            return {};
        }

        return {
                .size = {image.width(), image.height()},
                .bytesPerLine = static_cast<uint16_t>(image.bytes_per_line()),
                .format = convert(image.pixel_format()),
                .bpp = static_cast<uint8_t>(image.bits_per_pixel()),
                .hasPalette = image.palette().is_valid()
            };
    }


    TextureCreateInfo parseKtxTextureData(ktxTexture * texture) {
        if (ktxTexture_NeedsTranscoding(texture)) {
            if (texture->classId == class_id::ktxTexture2_c) {
                auto * texture2 = reinterpret_cast<ktxTexture2*>(texture);

                // ktxTexture2_GetNumComponents(ktxTexture);
                // ktxTexture2_GetPremultipliedAlpha(ktxTexture);
            }
            // todo!
            return {};
        } else {
            auto const format = ktxTexture_GetVkFormat(texture);
            ktx_size_t imageOffset = 0u;
            const auto imageOffsetGetResult = ktxTexture_GetImageOffset(texture, 0u, 0u, 0u, &imageOffset);
            std::span data{reinterpret_cast<std::byte*>(ktxTexture_GetData(texture)) + imageOffset, ktxTexture_GetImageSize(texture, 0u)};

            return {
                .metaInfo = {
                    .size = {texture->baseWidth, texture->baseHeight},
                    .bytesPerLine = static_cast<uint16_t>(ktxTexture_GetRowPitch(texture, 0u)),
                    .format = convert(format),
                    .bpp = 0,
                    .hasPalette = texture->_protected->_formatSize.paletteSizeInBits > 0u
                },
                .data = {data.begin(), data.end()}
            };
        }
    }

    TextureCreateInfo parseSailTextureData(std::span<const std::byte> bytes) {
        sail::image_input imageInput(bytes.data(), bytes.size());
        sail::image image = imageInput.next_frame();

        if (!image.is_valid()) {
            return {};
        }
        
        std::span data{static_cast<std::byte*>(image.pixels()), image.pixels_size()}; 

        return {
            .metaInfo = {
                .size = {image.width(), image.height()},
                .bytesPerLine = static_cast<uint16_t>(image.bytes_per_line()),
                .format = convert(image.pixel_format()),
                .bpp = static_cast<uint8_t>(image.bits_per_pixel()),
                .hasPalette = image.palette().is_valid()
            },
            .data = {data.begin(), data.end()}
        };
    }

    TextureCreateInfo parseTextureData(std::span<const std::byte> bytes) {
		ktxTexture * ktxTexture;
        // if data is no ktx - no load data, only check header, it is correct!
        ktxResult result = ktxTexture_CreateFromMemory(reinterpret_cast<const ktx_uint8_t*>(bytes.data()), bytes.size(), 
            ktxTextureCreateFlagBits::KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT | ktxTextureCreateFlagBits::KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT, &ktxTexture);

        if (result == ktxResult::KTX_SUCCESS) {
            return parseKtxTextureData(ktxTexture);
        } else if (result == ktxResult::KTX_UNKNOWN_FILE_FORMAT) {
            return parseSailTextureData(bytes);
        }
    }

    bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info) {
        const auto targetFormat = convert(format);
        sail::image image(info.data.data(), convert(info.metaInfo.format), info.metaInfo.size.x, info.metaInfo.size.y, info.metaInfo.bytesPerLine);

        if (image.can_convert(targetFormat)) {
            if (image.convert(targetFormat) == SAIL_OK) {
                info.metaInfo.format = format;
                info.metaInfo.bpp = image.bits_per_pixel();
                info.metaInfo.bytesPerLine = image.bytes_per_line();
                std::span data{static_cast<std::byte*>(image.pixels()), image.pixels_size()};
                info.data = {data.begin(), data.end()};
                return true;
            }
        }

        return false;
    }

}