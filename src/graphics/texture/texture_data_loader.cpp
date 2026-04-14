#include "texture_data_loader.h"

#include "log/log.h"
// ktx
#include <ktx.h> // KTX-Software/include
#include <ktxint.h>
#include <texture.h> // KTX-Software/include
#include <texture2.h> // KTX-Software/include
#include <vk_format.h> // KTX-Software/lib/ // this header include gl_format.h and conversions gl to vk

// sail
#include <sail-c++/sail-c++.h>

// common
#include <cassert>

namespace engine::graphics {
namespace ktx_format {
    rhi::Format convert(VkFormat format, TextureFlags flags) noexcept {
        const bool allowSrgb = (flags & TextureFlags::eAllowSrgbColorSpace) != TextureFlags::eNone;

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
            case VK_FORMAT_R8_SRGB: return allowSrgb ? rhi::Format::eR8_Srgb : rhi::Format::eR8_Unorm;
            case VK_FORMAT_R8G8_UNORM: return rhi::Format::eR8G8_Unorm;
            case VK_FORMAT_R8G8_SNORM: return rhi::Format::eR8G8_Snorm;
            case VK_FORMAT_R8G8_USCALED: return rhi::Format::eR8G8_Uscaled;
            case VK_FORMAT_R8G8_SSCALED: return rhi::Format::eR8G8_Sscaled;
            case VK_FORMAT_R8G8_UINT: return rhi::Format::eR8G8_Uint;
            case VK_FORMAT_R8G8_SINT: return rhi::Format::eR8G8_Sint;
            case VK_FORMAT_R8G8_SRGB: return allowSrgb ? rhi::Format::eR8G8_Srgb : rhi::Format::eR8G8_Unorm;
            case VK_FORMAT_R8G8B8_UNORM: return rhi::Format::eR8G8B8_Unorm;
            case VK_FORMAT_R8G8B8_SNORM: return rhi::Format::eR8G8B8_Snorm;
            case VK_FORMAT_R8G8B8_USCALED: return rhi::Format::eR8G8B8_Uscaled;
            case VK_FORMAT_R8G8B8_SSCALED: return rhi::Format::eR8G8B8_Sscaled;
            case VK_FORMAT_R8G8B8_UINT: return rhi::Format::eR8G8B8_Uint;
            case VK_FORMAT_R8G8B8_SINT: return rhi::Format::eR8G8B8_Sint;
            case VK_FORMAT_R8G8B8_SRGB: return allowSrgb ? rhi::Format::eR8G8B8_Srgb : rhi::Format::eR8G8B8_Unorm;
            case VK_FORMAT_B8G8R8_UNORM: return rhi::Format::eB8G8R8_Unorm;
            case VK_FORMAT_B8G8R8_SNORM: return rhi::Format::eB8G8R8_Snorm;
            case VK_FORMAT_B8G8R8_USCALED: return rhi::Format::eB8G8R8_Uscaled;
            case VK_FORMAT_B8G8R8_SSCALED: return rhi::Format::eB8G8R8_Sscaled;
            case VK_FORMAT_B8G8R8_UINT: return rhi::Format::eB8G8R8_Uint;
            case VK_FORMAT_B8G8R8_SINT: return rhi::Format::eB8G8R8_Sint;
            case VK_FORMAT_B8G8R8_SRGB: return allowSrgb ? rhi::Format::eB8G8R8_Srgb : rhi::Format::eB8G8R8_Unorm;
            case VK_FORMAT_R8G8B8A8_UNORM: return rhi::Format::eR8G8B8A8_Unorm;
            case VK_FORMAT_R8G8B8A8_SNORM: return rhi::Format::eR8G8B8A8_Snorm;
            case VK_FORMAT_R8G8B8A8_USCALED: return rhi::Format::eR8G8B8A8_Uscaled;
            case VK_FORMAT_R8G8B8A8_SSCALED: return rhi::Format::eR8G8B8A8_Sscaled;
            case VK_FORMAT_R8G8B8A8_UINT: return rhi::Format::eR8G8B8A8_Uint;
            case VK_FORMAT_R8G8B8A8_SINT: return rhi::Format::eR8G8B8A8_Sint;
            case VK_FORMAT_R8G8B8A8_SRGB: return allowSrgb ? rhi::Format::eR8G8B8A8_Srgb : rhi::Format::eR8G8B8A8_Unorm;
            case VK_FORMAT_B8G8R8A8_UNORM: return rhi::Format::eB8G8R8A8_Unorm;
            case VK_FORMAT_B8G8R8A8_SNORM: return rhi::Format::eB8G8R8A8_Snorm;
            case VK_FORMAT_B8G8R8A8_USCALED: return rhi::Format::eB8G8R8A8_Uscaled;
            case VK_FORMAT_B8G8R8A8_SSCALED: return rhi::Format::eB8G8R8A8_Sscaled;
            case VK_FORMAT_B8G8R8A8_UINT: return rhi::Format::eB8G8R8A8_Uint;
            case VK_FORMAT_B8G8R8A8_SINT: return rhi::Format::eB8G8R8A8_Sint;
            case VK_FORMAT_B8G8R8A8_SRGB: return allowSrgb ? rhi::Format::eB8G8R8A8_Srgb : rhi::Format::eB8G8R8A8_Unorm;
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return rhi::Format::eA8B8G8R8_Unorm_Pack32;
            case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return rhi::Format::eA8B8G8R8_Snorm_Pack32;
            case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return rhi::Format::eA8B8G8R8_Uscaled_Pack32;
            case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return rhi::Format::eA8B8G8R8_Sscaled_Pack32;
            case VK_FORMAT_A8B8G8R8_UINT_PACK32: return rhi::Format::eA8B8G8R8_Uint_Pack32;
            case VK_FORMAT_A8B8G8R8_SINT_PACK32: return rhi::Format::eA8B8G8R8_Sint_Pack32;
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return allowSrgb ? rhi::Format::eA8B8G8R8_Srgb_Pack32 : rhi::Format::eA8B8G8R8_Unorm_Pack32;

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
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return allowSrgb ? rhi::Format::eBC1_RGB_Srgb_Block : rhi::Format::eBC1_RGB_Unorm_Block;
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return rhi::Format::eBC1_RGBA_Unorm_Block;
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return allowSrgb ? rhi::Format::eBC1_RGBA_Srgb_Block : rhi::Format::eBC1_RGBA_Unorm_Block;
            case VK_FORMAT_BC2_UNORM_BLOCK: return rhi::Format::eBC2_Unorm_Block;
            case VK_FORMAT_BC2_SRGB_BLOCK: return allowSrgb ? rhi::Format::eBC2_Srgb_Block : rhi::Format::eBC2_Unorm_Block;
            case VK_FORMAT_BC3_UNORM_BLOCK: return rhi::Format::eBC3_Unorm_Block;
            case VK_FORMAT_BC3_SRGB_BLOCK: return allowSrgb ? rhi::Format::eBC3_Srgb_Block : rhi::Format::eBC2_Unorm_Block;
            case VK_FORMAT_BC4_UNORM_BLOCK: return rhi::Format::eBC4_Unorm_Block;
            case VK_FORMAT_BC4_SNORM_BLOCK: return rhi::Format::eBC4_Snorm_Block;
            case VK_FORMAT_BC5_UNORM_BLOCK: return rhi::Format::eBC5_Unorm_Block;
            case VK_FORMAT_BC5_SNORM_BLOCK: return rhi::Format::eBC5_Snorm_Block;
            case VK_FORMAT_BC6H_UFLOAT_BLOCK: return rhi::Format::eBC6H_Ufloat_Block;
            case VK_FORMAT_BC6H_SFLOAT_BLOCK: return rhi::Format::eBC6H_Sfloat_Block;
            case VK_FORMAT_BC7_UNORM_BLOCK: return rhi::Format::eBC7_Unorm_Block;
            case VK_FORMAT_BC7_SRGB_BLOCK: return allowSrgb ? rhi::Format::eBC7_Srgb_Block : rhi::Format::eBC7_Unorm_Block;

            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return rhi::Format::eETC2_R8G8B8_Unorm_Block;
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return allowSrgb ? rhi::Format::eETC2_R8G8B8_Srgb_Block : rhi::Format::eETC2_R8G8B8_Unorm_Block;
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return rhi::Format::eETC2_R8G8B8A1_Unorm_Block;
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return allowSrgb ? rhi::Format::eETC2_R8G8B8A1_Srgb_Block : rhi::Format::eETC2_R8G8B8A1_Unorm_Block;
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return rhi::Format::eETC2_R8G8B8A8_Unorm_Block;
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return allowSrgb ? rhi::Format::eETC2_R8G8B8A8_Srgb_Block : rhi::Format::eETC2_R8G8B8A8_Unorm_Block;

            case VK_FORMAT_EAC_R11_UNORM_BLOCK: return rhi::Format::eEAC_R11_Unorm_Block;
            case VK_FORMAT_EAC_R11_SNORM_BLOCK: return rhi::Format::eEAC_R11_Snorm_Block;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return rhi::Format::eEAC_R11G11_Unorm_Block;
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return rhi::Format::eEAC_R11G11_Snorm_Block;

            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return rhi::Format::eASTC_4x4_Unorm_Block;
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_4x4_Srgb_Block : rhi::Format::eASTC_4x4_Unorm_Block;
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return rhi::Format::eASTC_5x4_Unorm_Block;
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_5x4_Srgb_Block: rhi::Format::eASTC_5x4_Unorm_Block;
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return rhi::Format::eASTC_5x5_Unorm_Block;
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_5x5_Srgb_Block: rhi::Format::eASTC_5x5_Unorm_Block;
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return rhi::Format::eASTC_6x5_Unorm_Block;
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_6x5_Srgb_Block: rhi::Format::eASTC_6x5_Unorm_Block;
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return rhi::Format::eASTC_6x6_Unorm_Block;
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_6x6_Srgb_Block: rhi::Format::eASTC_6x6_Unorm_Block;
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return rhi::Format::eASTC_8x5_Unorm_Block;
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_8x5_Srgb_Block: rhi::Format::eASTC_8x5_Unorm_Block;
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return rhi::Format::eASTC_8x6_Unorm_Block;
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_8x6_Srgb_Block: rhi::Format::eASTC_8x6_Unorm_Block;
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return rhi::Format::eASTC_8x8_Unorm_Block;
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_8x8_Srgb_Block: rhi::Format::eASTC_8x8_Unorm_Block;
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return rhi::Format::eASTC_10x5_Unorm_Block;
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_10x5_Srgb_Block: rhi::Format::eASTC_10x5_Unorm_Block;
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return rhi::Format::eASTC_10x6_Unorm_Block;
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_10x6_Srgb_Block: rhi::Format::eASTC_10x6_Unorm_Block;
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return rhi::Format::eASTC_10x8_Unorm_Block;
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_10x8_Srgb_Block: rhi::Format::eASTC_10x8_Unorm_Block;
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return rhi::Format::eASTC_10x10_Unorm_Block;
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_10x10_Srgb_Block: rhi::Format::eASTC_10x10_Unorm_Block;
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return rhi::Format::eASTC_12x10_Unorm_Block;
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_12x10_Srgb_Block: rhi::Format::eASTC_12x10_Unorm_Block;
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return rhi::Format::eASTC_12x12_Unorm_Block;
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return allowSrgb ? rhi::Format::eASTC_12x12_Srgb_Block: rhi::Format::eASTC_12x12_Unorm_Block;
            default: return rhi::Format::eUndefined;
        }

        assert(false); // unknown format
        return rhi::Format::eUndefined;
    }

    inline rhi::Format getFormat(ktxTexture * texture, TextureFlags flags) noexcept {
        switch (texture->classId) {
            case class_id::ktxTexture2_c: {
                auto * texture2 = reinterpret_cast<ktxTexture2*>(texture);
                return convert(static_cast<VkFormat>(texture2->vkFormat), flags);
            }
            case class_id::ktxTexture1_c: {
                auto * texture1 = reinterpret_cast<ktxTexture1*>(texture);
                // return convert(vkGetFormatFromOpenGLFormat(texture1->glBaseInternalformat, texture1->glType), flags);
                return convert(vkGetFormatFromOpenGLInternalFormat(texture1->glInternalformat), flags);
            }
            default: return rhi::Format::eUndefined;
        }
    }

    KTX_error_code levelCallback(int miplevel, int face, int width, int height, int depth, 
        ktx_uint64_t faceLodSize,
        void* pixels, void* userdata) {
        LOG_D("mip level: %d, face: %d, size: %u bytes\n", miplevel, face, faceLodSize);

        if (miplevel == 0u) {
            TextureCreateInfo & result = *static_cast<TextureCreateInfo*>(userdata);
            result.data = {static_cast<std::byte*>(pixels), static_cast<std::byte*>(pixels) + faceLodSize};
        }

        // Example: Upload 'pixels' to GPU here using glTexImage2D or vkCmdCopyBufferToImage
        return KTX_SUCCESS; // Return KTX_SUCCESS to continue, or error to stop
    }

    TextureCreateInfo parseTextureData(ktxTexture * texture, const TextureLoadInfo & info) {
        if (ktxTexture_NeedsTranscoding(texture)) {
            if (texture->classId == class_id::ktxTexture2_c) {
                auto * texture2 = reinterpret_cast<ktxTexture2*>(texture);
                // const auto numComponents = ktxTexture2_GetNumComponents(texture2);
                // const auto premultipliedAlpha = ktxTexture2_GetPremultipliedAlpha(texture2);
                
                // select target format! 
                // maybe need to check device format support for some rhi::FormatFeatureFlags?
                // or is this enough?
                #if (defined ENGINE_PLATFORM_LINUX) || (defined ENGINE_PLATFORM_WINDOWS)
                    const auto targetFormat = ktx_transcode_fmt_e::KTX_TTF_BC1_OR_3;
                #endif

                const auto result = ktxTexture2_TranscodeBasis(texture2, targetFormat, 0u);

                if (result == ktxResult::KTX_SUCCESS) {
                    ktx_size_t imageOffset = 0u;
                    const auto imageOffsetGetResult = ktxTexture2_GetImageOffset(texture2, 0u, 0u, 0u, &imageOffset);
                    std::span data{
                        reinterpret_cast<std::byte*>(texture2->pData) + imageOffset, 
                        ktxTexture_GetImageSize(texture, 0u)
                    };

                    TextureCreateInfo result {
                        .metaInfo = {
                            .size = {texture2->baseWidth, texture2->baseHeight, 1u},
                            .bytesPerLine = static_cast<uint16_t>(ktxTexture_GetRowPitch(reinterpret_cast<ktxTexture*>(texture2), 0u)),
                            .format = getFormat(reinterpret_cast<ktxTexture*>(texture2), info.flags),
                            .bpp = 0u,
                            .hasPalette = (texture2->_protected->_formatSize.flags & ktxFormatSizeFlagBits::KTX_FORMAT_SIZE_PALETTIZED_BIT) != 0,
                            .parserType = TextureLoaderType::eKtx,
                            .flags = info.flags
                        },
                        .data = {}
                    };
                    ktxTexture2_IterateLevels(texture2, levelCallback, &result);
                    return result;
                    // return {
                    //     .metaInfo = {
                    //         .size = {texture2->baseWidth, texture2->baseHeight, 1u},
                    //         .bytesPerLine = static_cast<uint16_t>(ktxTexture_GetRowPitch(reinterpret_cast<ktxTexture*>(texture2), 0u)),
                    //         .format = getFormat(reinterpret_cast<ktxTexture*>(texture2), info.flags),
                    //         .bpp = 0u,
                    //         .hasPalette = (texture2->_protected->_formatSize.flags & ktxFormatSizeFlagBits::KTX_FORMAT_SIZE_PALETTIZED_BIT) != 0,
                    //         .parserType = TextureLoaderType::eKtx,
                    //         .flags = info.flags
                    //     },
                    //     .data = {data.begin(), data.end()}
                    // };

                }
            }

            assert(false);
            return {};
        } else {
            ktx_size_t imageOffset = 0u;
            const auto imageOffsetGetResult = ktxTexture_GetImageOffset(texture, 0u, 0u, 0u, &imageOffset);
            std::span data{
                reinterpret_cast<std::byte*>(ktxTexture_GetData(texture)) + imageOffset, 
                ktxTexture_GetImageSize(texture, 0u)
            };

            TextureCreateInfo result {
                .metaInfo = {
                    .size = {texture->baseWidth, texture->baseHeight, 1u},
                    .bytesPerLine = static_cast<uint16_t>(ktxTexture_GetRowPitch(texture, 0u)),
                    .format = getFormat(texture, info.flags),
                    .bpp = 0u,
                    .hasPalette = (texture->_protected->_formatSize.flags & ktxFormatSizeFlagBits::KTX_FORMAT_SIZE_PALETTIZED_BIT) != 0,
                    .parserType = TextureLoaderType::eKtx,
                    .flags = info.flags
                },
                .data = {}
            };

            ktxTexture_IterateLevels(texture, levelCallback, &result);
            return result;
            // return {
            //     .metaInfo = {
            //         .size = {texture->baseWidth, texture->baseHeight, 1u},
            //         .bytesPerLine = static_cast<uint16_t>(ktxTexture_GetRowPitch(texture, 0u)),
            //         .format = getFormat(texture, info.flags),
            //         .bpp = 0u,
            //         .hasPalette = (texture->_protected->_formatSize.flags & ktxFormatSizeFlagBits::KTX_FORMAT_SIZE_PALETTIZED_BIT) != 0,
            //         .parserType = TextureLoaderType::eKtx,
            //         .flags = info.flags
            //     },
            //     .data = {data.begin(), data.end()}
            // };
        }
    }

    TextureMetaInfo parseTextureMetaInfoOnly(ktxTexture * texture, const TextureLoadInfo & info) {
        return {
            .size = {texture->baseWidth, texture->baseHeight, 1u},
            .bytesPerLine = static_cast<uint16_t>(ktxTexture_GetRowPitch(texture, 0u)),
            .format = getFormat(texture, info.flags),
            .bpp = 0,
            .hasPalette = (texture->_protected->_formatSize.flags & ktxFormatSizeFlagBits::KTX_FORMAT_SIZE_PALETTIZED_BIT) != 0,
            .parserType = TextureLoaderType::eKtx,
            .flags = info.flags
        };
    }
}

namespace sail_format {
    rhi::Format convert(SailPixelFormat format, TextureFlags flags) noexcept {
        const bool allowSrgb = (flags & TextureFlags::eAllowSrgbColorSpace) != TextureFlags::eNone;

        switch (format) {
        // Indexed formats.
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_INDEXED: return rhi::Format::eR8_Unorm;

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
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_RGB: return allowSrgb ? rhi::Format::eR8G8B8_Srgb : rhi::Format::eR8G8B8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_BGR: return allowSrgb ? rhi::Format::eB8G8R8_Srgb : rhi::Format::eB8G8R8_Unorm;

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

        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBX: return allowSrgb ? rhi::Format::eR8G8B8A8_Srgb : rhi::Format::eR8G8B8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRX: return allowSrgb ? rhi::Format::eB8G8R8A8_Srgb : rhi::Format::eB8G8R8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_XRGB: return rhi::Format::eUndefined; // !
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_XBGR: return rhi::Format::eA8B8G8R8_Unorm_Pack32;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBA: return allowSrgb ? rhi::Format::eR8G8B8A8_Srgb : rhi::Format::eR8G8B8A8_Unorm;
        case SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRA: return allowSrgb ? rhi::Format::eB8G8R8A8_Srgb : rhi::Format::eB8G8R8A8_Unorm;
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

    SailPixelFormat convert(rhi::Format format, bool palette) noexcept {
        switch (format) {
        // Grayscale formats.
        case rhi::Format::eR8_Unorm: return palette ? 
                                                    SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_INDEXED : 
                                                    SailPixelFormat::SAIL_PIXEL_FORMAT_BPP8_GRAYSCALE;
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

        case rhi::Format::eR8G8B8_Srgb: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_RGB;
        case rhi::Format::eB8G8R8_Srgb: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP24_BGR;

        // RGBA/X formats. X = unused color channel.
        case rhi::Format::eR4G4B4A4_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_RGBA;
        case rhi::Format::eB4G4R4A4_Unorm_Pack16: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP16_BGRA;

        case rhi::Format::eR8G8B8A8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBA;
        case rhi::Format::eB8G8R8A8_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRA;
        case rhi::Format::eA8B8G8R8_Unorm_Pack32: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_ABGR;

        case rhi::Format::eR8G8B8A8_Srgb: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_RGBA;
        case rhi::Format::eB8G8R8A8_Srgb: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP32_BGRA;

        case rhi::Format::eR16G16B16A16_Unorm: return SailPixelFormat::SAIL_PIXEL_FORMAT_BPP64_RGBA;
        
        default: SailPixelFormat::SAIL_PIXEL_FORMAT_UNKNOWN;
        }
        return SailPixelFormat::SAIL_PIXEL_FORMAT_UNKNOWN;
    }

    TextureCreateInfo parseTextureData(std::span<const std::byte> bytes, const TextureLoadInfo & info) {
        sail::image_input imageInput(bytes.data(), bytes.size());
        sail::image image = imageInput.next_frame();

        if (!image.is_valid()) {
            return {};
        }
        
        std::span data{static_cast<std::byte*>(image.pixels()), image.pixels_size()}; 

// if (image.is_indexed()) { // palleted
//     const auto targetFormat = convert(rhi::Format::eR8G8B8A8_Unorm, false);
//     if (image.can_convert(targetFormat)) {
//             if (image.convert(targetFormat) == SAIL_OK) {

//             std::span data{static_cast<std::byte*>(image.pixels()), image.pixels_size()};

//         return {
//             .metaInfo = {
//                 .size = {image.width(), image.height(), 1u},
//                 .bytesPerLine = static_cast<uint16_t>(image.bytes_per_line()),
//                 .format = convert(image.pixel_format()),
//                 .bpp = static_cast<uint8_t>(image.bits_per_pixel()),
//                 .hasPalette = image.palette().is_valid(),
//                 .parserType = TextureLoaderType::eSail,
//                 .flags = info.flags
//             },
//             .data = {data.begin(), data.end()}
//         };
//             }
//         }
// }

// void set_palette(sail::palette &&palette) noexcept;

        auto & palette = image.palette();
        palette.data();

        return {
            .metaInfo = {
                .size = {image.width(), image.height(), 1u},
                .bytesPerLine = static_cast<uint16_t>(image.bytes_per_line()),
                .format = convert(image.pixel_format(), info.flags),
                .bpp = static_cast<uint8_t>(image.bits_per_pixel()),
                .hasPalette = image.is_indexed(), // image.palette().is_valid(),
                .parserType = TextureLoaderType::eSail,
                .flags = info.flags
            },
            .data = {data.begin(), data.end()}
        };
    }

    TextureMetaInfo parseTextureMetaInfoOnly(std::span<const std::byte> bytes, const TextureLoadInfo & info) {
        sail::image_input imageInput(bytes.data(), bytes.size());
        auto [image, _] = imageInput.probe();

        if (!image.is_valid()) {
            return {};
        }

        return {
            .size = {image.width(), image.height(), 1u},
            .bytesPerLine = static_cast<uint16_t>(image.bytes_per_line()),
            .format = convert(image.pixel_format(), info.flags),
            .bpp = static_cast<uint8_t>(image.bits_per_pixel()),
            .hasPalette = image.palette().is_valid(),
            .parserType = TextureLoaderType::eSail,
            .flags = info.flags
        };
    }

    bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info) {
        if (info.metaInfo.format == format) {
            return true;
        }

        const auto targetFormat = convert(format, false);
        sail::image image(info.data.data(), convert(info.metaInfo.format, info.metaInfo.hasPalette), 
            info.metaInfo.size.x, info.metaInfo.size.y, info.metaInfo.bytesPerLine);

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

    struct KtxTextureHandler {
        ~KtxTextureHandler() {
            if (texture) {
                ktxTexture_Destroy(texture);
            }
        }
        ktxTexture * texture = nullptr;
    };

    TextureCreateInfo parseTextureData(std::span<const std::byte> bytes, const TextureLoadInfo & info) {
        sail::log::set_barrier(SAIL_LOG_LEVEL_WARNING);
		KtxTextureHandler textureHandler;
        // if data is no ktx - no load data, only check header, it is correct!
        const auto flags = ktxTextureCreateFlagBits::KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT |
                           ktxTextureCreateFlagBits::KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT;

        const ktxResult result = ktxTexture_CreateFromMemory(reinterpret_cast<const ktx_uint8_t*>(bytes.data()), 
                                                       bytes.size(), flags, &textureHandler.texture);

        if (result == ktxResult::KTX_SUCCESS) {
            return ktx_format::parseTextureData(textureHandler.texture, info);
        } else if (result == ktxResult::KTX_UNKNOWN_FILE_FORMAT) {
            return sail_format::parseTextureData(bytes, info);
        }

        assert(false);
        return {};
    }

    TextureMetaInfo parseTextureMetaInfoOnly(std::span<const std::byte> bytes, const TextureLoadInfo & info) {
        KtxTextureHandler textureHandler;
        // if data is no ktx - no load data, only check header, it is correct!
        const auto flags = ktxTextureCreateFlagBits::KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT;

        const ktxResult result = ktxTexture_CreateFromMemory(reinterpret_cast<const ktx_uint8_t*>(bytes.data()), 
                                                       bytes.size(), flags, &textureHandler.texture);

        if (result == ktxResult::KTX_SUCCESS) {
            return ktx_format::parseTextureMetaInfoOnly(textureHandler.texture, info);
        } else if (result == ktxResult::KTX_UNKNOWN_FILE_FORMAT) {
            return sail_format::parseTextureMetaInfoOnly(bytes, info);
        }

        assert(false);
        return {};
    }

    bool convertTextureDataFormat(rhi::Format format, TextureCreateInfo & info) {

        // if (info.metaInfo.format == rhi::Format::eR8G8B8_Unorm) {
        //     ktxTextureCreateInfo createInfo = {0};
        //     createInfo.baseWidth = info.metaInfo.size.x;
        //     createInfo.baseHeight = info.metaInfo.size.y;
        //     createInfo.baseDepth = 1; // 2D texture
        //     createInfo.numDimensions = 2;
        //     createInfo.numLevels = 1; // No mipmaps initially
        //     createInfo.numLayers = 1;
        //     createInfo.numFaces = 1;
        //     createInfo.isArray = KTX_FALSE;
        //     createInfo.generateMipmaps = KTX_FALSE;
        //     createInfo.vkFormat = VK_FORMAT_R8G8B8_UNORM;
            
        //     // Set appropriate format based on 'channels'
        //     // createInfo.vkFormat == VK_FORMAT_R8G8B8_UNORM;//convert(rhi::Format::eBC7_Unorm_Block);
        //     // if (channels == 4) {
        //     //     createInfo.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        //     // } else if (channels == 3) {
        //     //     createInfo.vkFormat = VK_FORMAT_R8G8B8_UNORM;
        //     // }
        //     // ... set other fields

        //     ktxTexture2* kTexture; // Use ktxTexture for v1
        //     KTX_error_code result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &kTexture);
        //     if (result != KTX_SUCCESS) {
        //         // Handle error
        //     }

        //     result = ktxTexture_SetImageFromMemory(
        //         (ktxTexture*)kTexture, // Cast if using ktxTexture2 functions with generic pointer
        //         0, // level
        //         0, // layer
        //         0, // faceSlice
        //         reinterpret_cast<ktx_uint8_t*>(info.data.data()),
        //         info.data.size() // size of the image data
        //     );
        //     if (result != KTX_SUCCESS) {
        //         // Handle error
        //     }
        // }

        switch (info.metaInfo.parserType) {
            case TextureLoaderType::eSail: return sail_format::convertTextureDataFormat(format, info);
            case TextureLoaderType::eKtx: return false;
            case TextureLoaderType::eRaw: return false;
            default: return false;
        }
    }
}