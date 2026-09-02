#pragma once

#include "common/hash.h"
#include "utils/dynamic_array.h"
#include "stl/algorithm.h"
#include "stl/array.h"
#include "stl/utility.h"
#include "stl/vector.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace engine::rhi {
    // template <typename T>
    // inline T operator | (T a, T b) {
    //     return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
    // }

    // template <typename T>
    // inline T operator & (T a, T b) {
    //     return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
    // }

    inline constexpr size_t kWholeSize = std::numeric_limits<size_t>::max();
    inline constexpr uint64_t kWholeSize64 = ~0ULL;
    inline constexpr uint32_t kWholeSize32 = (~0U);

    struct Offset2D {
        int32_t x {};
        int32_t y {};

        inline bool operator == (const Offset2D & offset) const noexcept = default;
    };
    
    struct Offset3D {
        int32_t x {};
        int32_t y {};
        int32_t z {};

        inline bool operator == (const Offset3D & offset) const noexcept = default;
    };
    
    struct Extent2D {
        uint32_t width = 0u;
        uint32_t height = 0u;

        inline bool operator == (const Extent2D & extent) const noexcept = default;
    };

    struct Extent3D {
        uint32_t width = 0u;
        uint32_t height = 0u;
        uint32_t depth = 0u;

        inline bool operator == (const Extent3D & extent) const noexcept = default;
    };
    
    struct Rect2D {
        Offset2D offset{};
        Extent2D extent{};

        inline bool operator == (const Rect2D & rect) const noexcept = default;
    };

    struct Viewport {
        Rect2D screenRect{};
        float minDepth = 0.0f;
        float maxDepth = 0.0f;

        inline bool operator == (const Viewport & viewport) const noexcept = default;
    };

    union ClearColorValue {
        float fvalue[4];
        int32_t ivalue[4];
        uint32_t uvalue[4];
    };

    struct ClearDepthStencilValue {
        float depth = 1.0f;
        uint32_t stencil = 0u;
    };
    
    union ClearValue {
        ClearColorValue color;
        ClearDepthStencilValue depthStencil;

        constexpr ClearValue() : depthStencil{1.0f, 0u} {}
        
        explicit constexpr ClearValue(uint32_t color) : 
        color{
            static_cast<float>(0xffu & (color >> 24u)) / 255.0f, 
            static_cast<float>(0xffu & (color >> 16u)) / 255.0f, 
            static_cast<float>(0xffu & (color >> 8u)) / 255.0f, 
            static_cast<float>(0xffu & (color >> 0u)) / 255.0f
        } {}

        constexpr ClearValue(float r, float g, float b, float a) :  color{r, g, b, a} {}   
        constexpr ClearValue(float depth, uint32_t stencil = 0u) : depthStencil{depth, stencil} {}
    };

    struct CopyBufferInfo {
        size_t srcOffset = 0u;
        size_t dstOffset = 0u;
        size_t size = 0u;
    };

    enum class Result : int8_t {
        eSuccess = 0u,
    };

    enum class MemoryPropertyFlags : uint8_t {
        eUndefined = 0u,
        eDeviceLocal = 1u,
        eHostVisible = 2u,
        eHostCoherent = 4u,
        eHostCached = 8u,
        eLazilyAllocated = 16u,
        eProtected = 32u
    };
    inline MemoryPropertyFlags operator | (MemoryPropertyFlags a, MemoryPropertyFlags b) {
        return static_cast<MemoryPropertyFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline MemoryPropertyFlags operator & (MemoryPropertyFlags a, MemoryPropertyFlags b) {
        return static_cast<MemoryPropertyFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class MemoryExtendedFlags : uint8_t {
        eNone = 0u,
        eDedicated = 1u, // Use it for special, big resources, like fullscreen images used as attachments.
        eTry_No_Allocate_New = 2u,
        eMapped = 4u,
        eCan_Alias = 8u
    };
    inline MemoryExtendedFlags operator | (MemoryExtendedFlags a, MemoryExtendedFlags b) {
        return static_cast<MemoryExtendedFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline MemoryExtendedFlags operator & (MemoryExtendedFlags a, MemoryExtendedFlags b) {
        return static_cast<MemoryExtendedFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class ColorComponent : uint8_t {
        eNone = 0u,
        eR = 1u,
        eG = 2u,
        eB = 4u, 
        eA = 8u
    };
    inline constexpr ColorComponent operator | (ColorComponent a, ColorComponent b) {
        return static_cast<ColorComponent>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline constexpr ColorComponent operator & (ColorComponent a, ColorComponent b) {
        return static_cast<ColorComponent>(std::to_underlying(a) & std::to_underlying(b));
    }
    
    enum class SharingMode : uint8_t {
        eExclusive = 0u,
        eConcurrent = 1u
    };

    enum class Format : uint8_t {
        eUndefined = 0u,
        eR4G4_Unorm_Pack8 = 1u,
        eR4G4B4A4_Unorm_Pack16 = 2u,
        eB4G4R4A4_Unorm_Pack16 = 3u,
        eR5G6B5_Unorm_Pack16 = 4u,
        eB5G6R5_Unorm_Pack16 = 5u,
        eR5G5B5A1_Unorm_Pack16 = 6u,
        eB5G5R5A1_Unorm_Pack16 = 7u,
        eA1R5G5B5_Unorm_Pack16 = 8u,

        eR8_Unorm = 9u,
        eR8_Snorm = 10u,
        eR8_Uscaled = 11u,
        eR8_Sscaled = 12u,
        eR8_Uint = 13u,
        eR8_Sint = 14u,
        eR8_Srgb = 15u,
        eR8G8_Unorm = 16u,
        eR8G8_Snorm = 17u,
        eR8G8_Uscaled = 18u,
        eR8G8_Sscaled = 19u,
        eR8G8_Uint = 20u,
        eR8G8_Sint = 21u,
        eR8G8_Srgb = 22u,
        eR8G8B8_Unorm = 23u,
        eR8G8B8_Snorm = 24u,
        eR8G8B8_Uscaled = 25u,
        eR8G8B8_Sscaled = 26u,
        eR8G8B8_Uint = 27u,
        eR8G8B8_Sint = 28u,
        eR8G8B8_Srgb = 29u,
        eB8G8R8_Unorm = 30u,
        eB8G8R8_Snorm = 31u,
        eB8G8R8_Uscaled = 32u,
        eB8G8R8_Sscaled = 33u,
        eB8G8R8_Uint = 34u,
        eB8G8R8_Sint = 35u,
        eB8G8R8_Srgb = 36u,
        eR8G8B8A8_Unorm = 37u,
        eR8G8B8A8_Snorm = 38u,
        eR8G8B8A8_Uscaled = 39u,
        eR8G8B8A8_Sscaled = 40u,
        eR8G8B8A8_Uint = 41u,
        eR8G8B8A8_Sint = 42u,
        eR8G8B8A8_Srgb = 43u,
        eB8G8R8A8_Unorm = 44u,
        eB8G8R8A8_Snorm = 45u,
        eB8G8R8A8_Uscaled = 46u,
        eB8G8R8A8_Sscaled = 47u,
        eB8G8R8A8_Uint = 48u,
        eB8G8R8A8_Sint = 49u,
        eB8G8R8A8_Srgb = 50u,
        eA8B8G8R8_Unorm_Pack32 = 51u,
        eA8B8G8R8_Snorm_Pack32 = 52u,
        eA8B8G8R8_Uscaled_Pack32 = 53u,
        eA8B8G8R8_Sscaled_Pack32 = 54u,
        eA8B8G8R8_Uint_Pack32 = 55u,
        eA8B8G8R8_Sint_Pack32 = 56u,
        eA8B8G8R8_Srgb_Pack32 = 57u,

        eA2R10G10B10_Unorm_Pack32 = 58u,
        eA2R10G10B10_Snorm_Pack32 = 59u,
        eA2R10G10B10_Uscaled_Pack32 = 60u,
        eA2R10G10B10_Sscaled_Pack32 = 61u,
        eA2R10G10B10_Uint_Pack32 = 62u,
        eA2R10G10B10_Sint_Pack32 = 63u,
        eA2B10G10R10_Unorm_Pack32 = 64u,
        eA2B10G10R10_Snorm_Pack32 = 65u,
        eA2B10G10R10_Uscaled_Pack32 = 66u,
        eA2B10G10R10_Sscaled_Pack32 = 67u,
        eA2B10G10R10_Uint_Pack32 = 68u,
        eA2B10G10R10_Sint_Pack32 = 69u,

        eR16_Unorm = 70u,
        eR16_Snorm = 71u,
        eR16_Uscaled = 72u,
        eR16_Sscaled = 73u,
        eR16_Uint = 74u,
        eR16_Sint = 75u,
        eR16_Sfloat = 76u,
        eR16G16_Unorm = 77u,
        eR16G16_Snorm = 78u,
        eR16G16_Uscaled = 79u,
        eR16G16_Sscaled = 80u,
        eR16G16_Uint = 81u,
        eR16G16_Sint = 82u,
        eR16G16_Sfloat = 83u,
        eR16G16B16_Unorm = 84u,
        eR16G16B16_Snorm = 85u,
        eR16G16B16_Uscaled = 86u,
        eR16G16B16_Sscaled = 87u,
        eR16G16B16_Uint = 88u,
        eR16G16B16_Sint = 89u,
        eR16G16B16_Sfloat = 90u,
        eR16G16B16A16_Unorm = 91u,
        eR16G16B16A16_Snorm = 92u,
        eR16G16B16A16_Uscaled = 93u,
        eR16G16B16A16_Sscaled = 94u,
        eR16G16B16A16_Uint = 95u,
        eR16G16B16A16_Sint = 96u,
        eR16G16B16A16_Sfloat = 97u,

        eR32_Uint = 98u,
        eR32_Sint = 99u,
        eR32_Sfloat = 100u,
        eR32G32_Uint = 101u,
        eR32G32_Sint = 102u,
        eR32G32_Sfloat = 103u,
        eR32G32B32_Uint = 104u,
        eR32G32B32_Sint = 105u,
        eR32G32B32_Sfloat = 106u,
        eR32G32B32A32_Uint = 107u,
        eR32G32B32A32_Sint = 108u,
        eR32G32B32A32_Sfloat = 109u,

        eR64_Uint = 110u,
        eR64_Sint = 111u,
        eR64_Sfloat = 112u,
        eR64G64_Uint = 113u,
        eR64G64_Sint = 114u,
        eR64G64_Sfloat = 115u,
        eR64G64B64_Uint = 116u,
        eR64G64B64_Sint = 117u,
        eR64G64B64_Sfloat = 118u,
        eR64G64B64A64_Uint = 119u,
        eR64G64B64A64_Sint = 120u,
        eR64G64B64A64_Sfloat = 121u,

        eB10G11R11_Ufloat_Pack32 = 122u,
        eE5B9G9R9_Ufloat_Pack32 = 123u,
        eD16_Unorm = 124u,
        eX8_D24_Unorm_Pack32 = 125u,
        eD32_Sfloat = 126u,
        eS8_Uint = 127u,
        eD16_Unorm_S8_Uint = 128u,
        eD24_Unorm_S8_Uint = 129u,
        eD32_Sfloat_S8_Uint = 130u,

        eBC1_RGB_Unorm_Block = 131u,
        eBC1_RGB_Srgb_Block = 132u,
        eBC1_RGBA_Unorm_Block = 133u,
        eBC1_RGBA_Srgb_Block = 134u,
        eBC2_Unorm_Block = 135u,
        eBC2_Srgb_Block = 136u,
        eBC3_Unorm_Block = 137u,
        eBC3_Srgb_Block = 138u,
        eBC4_Unorm_Block = 139u,
        eBC4_Snorm_Block = 140u,
        eBC5_Unorm_Block = 141u,
        eBC5_Snorm_Block = 142u,
        eBC6H_Ufloat_Block = 143u,
        eBC6H_Sfloat_Block = 144u,
        eBC7_Unorm_Block = 145u,
        eBC7_Srgb_Block = 146u,

        eETC2_R8G8B8_Unorm_Block = 147u,
        eETC2_R8G8B8_Srgb_Block = 148u,
        eETC2_R8G8B8A1_Unorm_Block = 149u,
        eETC2_R8G8B8A1_Srgb_Block = 150u,
        eETC2_R8G8B8A8_Unorm_Block = 151u,
        eETC2_R8G8B8A8_Srgb_Block = 152u,

        eEAC_R11_Unorm_Block = 153u,
        eEAC_R11_Snorm_Block = 154u,
        eEAC_R11G11_Unorm_Block = 155u,
        eEAC_R11G11_Snorm_Block = 156u,

        eASTC_4x4_Unorm_Block = 157u,
        eASTC_4x4_Srgb_Block = 158u,
        eASTC_5x4_Unorm_Block = 159u,
        eASTC_5x4_Srgb_Block = 160u,
        eASTC_5x5_Unorm_Block = 161u,
        eASTC_5x5_Srgb_Block = 162u,
        eASTC_6x5_Unorm_Block = 163u,
        eASTC_6x5_Srgb_Block = 164u,
        eASTC_6x6_Unorm_Block = 165u,
        eASTC_6x6_Srgb_Block = 166u,
        eASTC_8x5_Unorm_Block = 167u,
        eASTC_8x5_Srgb_Block = 168u,
        eASTC_8x6_Unorm_Block = 169u,
        eASTC_8x6_Srgb_Block = 170u,
        eASTC_8x8_Unorm_Block = 171u,
        eASTC_8x8_Srgb_Block = 172u,
        eASTC_10x5_Unorm_Block = 173u,
        eASTC_10x5_Srgb_Block = 174u,
        eASTC_10x6_Unorm_Block = 175u,
        eASTC_10x6_Srgb_Block = 176u,
        eASTC_10x8_Unorm_Block = 177u,
        eASTC_10x8_Srgb_Block = 178u,
        eASTC_10x10_Unorm_Block = 179u,
        eASTC_10x10_Srgb_Block = 180u,
        eASTC_12x10_Unorm_Block = 181u,
        eASTC_12x10_Srgb_Block = 182u,
        eASTC_12x12_Unorm_Block = 183u,
        eASTC_12x12_Srgb_Block = 184u,
    };

    inline bool isDepthStencilFormat(Format format) noexcept {
        switch (format) {
            case Format::eD16_Unorm_S8_Uint: [[fallthrough]];
            case Format::eD24_Unorm_S8_Uint: [[fallthrough]];
            case Format::eD32_Sfloat_S8_Uint:
            return true;
        default:
            return false;
        }
    }

    inline bool isDepthFormat(Format format) noexcept {
        switch (format) {
            case Format::eD16_Unorm: [[fallthrough]];
            case Format::eX8_D24_Unorm_Pack32: [[fallthrough]];
            case Format::eD32_Sfloat: [[fallthrough]];
            case Format::eS8_Uint: [[fallthrough]];
            case Format::eD16_Unorm_S8_Uint: [[fallthrough]];
            case Format::eD24_Unorm_S8_Uint: [[fallthrough]];
            case Format::eD32_Sfloat_S8_Uint:
            return true;
        default:
            return false;
        }
    }

    inline bool isColorFormat(Format format) noexcept {
        return !isDepthFormat(format);
    }

    enum class FormatFeatureFlags : uint32_t {
        eNone = 0u,
        eFormatFeature_Sampled_Image = 0x00000001u,
        eFormatFeature_Storage_Image = 0x00000002u,
        eFormatFeature_Storage_Image_Atomic = 0x00000004u,
        eFormatFeature_Uniform_Texel_Buffer = 0x00000008u,
        eFormatFeature_Storage_Texel_Buffer = 0x00000010u,
        eFormatFeature_Storage_Texel_Buffer_Atomic = 0x00000020u,
        eFormatFeature_Vertex_Buffer = 0x00000040u,
        eFormatFeature_Color_Attachment = 0x00000080u,
        eFormatFeature_Color_Attachment_Blend = 0x00000100u,
        eFormatFeature_Depth_Stencil_Attachment = 0x00000200u,
        eFormatFeature_Blit_Src = 0x00000400u,
        eFormatFeature_BLit_Dst = 0x00000800u,
        eFormatFeature_Sampled_Image_Filter_Linear = 0x00001000u,
        eFormatFeature_Transfer_Src = 0x00004000u,
        eFormatFeature_Transfer_Dst = 0x00008000u,
        eFormatFeature_Midpoint_Chroma_Samples = 0x00020000u,
        eFormatFeature_Sampled_Image_YCBCR_Conversion_Linear_Filter = 0x00040000u,
        eFormatFeature_Sampled_Image_YCBCR_Conversion_Separate_Reconstruction_Filter = 0x00080000u,
        eFormatFeature_Sampled_Image_YCBCR_Conversion_Chroma_Reconstruction_Explicit = 0x00100000u,
        eFormatFeature_Sampled_Image_YCBCR_Conversion_Chroma_Reconstruction_Explicit_Forceable = 0x00200000u,
        eFormatFeature_Disjoint = 0x00400000u,
        eFormatFeature_Cosited_Chroma_Samples = 0x00800000u,
        eFormatFeature_Sampled_Image_Filter_Minmax = 0x00010000u,
        eFormatFeature_Acceleration_Structure_Vertex_Buffer = 0x20000000u,
        eFormatFeature_Sampled_Image_Filter_Cubic_Ext = 0x00002000u,
        eFormatFeature_Fragment_Density_Map_Ext = 0x01000000u,
        eFormatFeature_Fragment_Shading_Rate_Attachment = 0x40000000u,
    };
    inline FormatFeatureFlags operator | (FormatFeatureFlags a, FormatFeatureFlags b) {
        return static_cast<FormatFeatureFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline FormatFeatureFlags operator & (FormatFeatureFlags a, FormatFeatureFlags b) {
        return static_cast<FormatFeatureFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    struct FormatFeatureAccess {
        enum class Bit : uint8_t {
            eBufferFeaturesBit = 0u,
            eLinearTilingFeaturesBit = 1u,
            eOptimalTilingFeaturesBit = 2u
        };

        void setBit(Bit bit, bool value) noexcept {
            if (value) {
                features |= (1u << std::to_underlying(bit));
            } else {
                features &= ~(1u << std::to_underlying(bit));
            }
        }

        bool checkBit(Bit bit) const noexcept {
            return (features & 1u << std::to_underlying(bit)) != 0u;
        }

        uint8_t features = 0u;
    };

    enum class ColorSpace : uint8_t {
        eSRGB_Nonlinear = 0u,
        eDISPLAY_P3_Nonlinear = 1u,
        eEXTENDED_SRGB_linear = 2u,
        eDISPLAY_P3_linear = 3u,
        eDCI_P3_Nonlinear = 4u,
        eBT709_linear = 5u,
        eBT709_Nonlinear = 6u,
        eBT2020_linear = 7u,
        eHDR10_ST2084 = 8u,
        eDOLBYVISION = 9u,
        eHDR10_HLG = 10u,
        eADOBERGB_linear = 11u,
        eADOBERGB_Nonlinear = 12u,
        ePASS_THROUGH = 13u,
        eEXTENDED_SRGB_Nonlinear = 14u,
        eDCI_P3_linear = 15u
    };

    enum class PresentMode : uint8_t {
        eImmidiate = 0u,
        eMailbox = 1u,
        eFifo = 2u
    };

    enum class BufferUsageFlags : uint32_t {
        eNone = 0u,
        eTransfer_Src = 0x00000001u,
        eTransfer_Dst = 0x00000002u,
        eUniform_Texel_Buffer = 0x00000004u,
        eStorage_Texel_Buffer = 0x00000008u,
        eUniform_Buffer = 0x00000010u,
        eStorage_Buffer = 0x00000020u,
        eIndex_Buffer = 0x00000040u,
        eVertex_Buffer = 0x00000080u,
        eIndirect_Buffer = 0x00000100u,
        eShader_Device_Address = 0x00020000u,
        eVideo_Decode_Src_Khr = 0x00002000u,
        eVideo_Decode_Dst_Khr = 0x00004000u,
        eTransform_Feedback_Buffer_Ext = 0x00000800u,
        eTransform_Feedback_Counter_Buffer_Ext = 0x00001000u,
        eConditional_Rendering_Ext = 0x00000200u,
        eAcceleration_Structure_Build_Input_Read_Only_Khr = 0x00080000u,
        eAcceleration_Structure_Storage_Khr = 0x00100000u,
        eShader_Binding_Table_Khr = 0x00000400u, // KHR_ray_tracing_pipeline
        eVideo_Encode_Dst_Khr = 0x00008000u,
        eVideo_Encode_Src_Khr = 0x00010000u,
        eSampler_Descriptor_Buffer_Ext = 0x00200000u, // EXT_descriptor_buffer
        eResource_Descriptor_Buffer_Ext = 0x00400000u, // EXT_descriptor_buffer
        ePush_Descriptors_Descriptor_Buffer_Ext = 0x04000000u, // EXT_descriptor_buffer
        eMicromap_Build_Input_Read_Only_Ext = 0x00800000u, // EXT_opacity_micromap
        eMicromap_Storage_Ext = 0x01000000u, // EXT_opacity_micromap
        eRay_Tracing_NV = eShader_Binding_Table_Khr,
    };
    inline BufferUsageFlags operator | (BufferUsageFlags a, BufferUsageFlags b) {
        return static_cast<BufferUsageFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline BufferUsageFlags operator & (BufferUsageFlags a, BufferUsageFlags b) {
        return static_cast<BufferUsageFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class ImageType : uint8_t {
        eType_1D = 0u,
        eType_2D = 1u,
        eType_3D = 2u,
    };

    enum class ImageViewType : uint8_t {
        eType_1D = 0u,
        eType_2D = 1u,
        eType_3D = 2u,
        eType_Cube = 3u,
        eType_1D_Array = 4u,
        eType_2D_Array = 5u,
        eType_Cube_Array = 6u,
        eType_Undefined = 0xffu,
    };

    enum class ImageAspectFlags : uint8_t {
        eNone = 0u,
        eColor = 1u,
        eDepth = 2u,
        eStencil = 4u,
        eMetadata = 8u,
        eDepthStencil = eDepth | eStencil
    };
    inline ImageAspectFlags operator | (ImageAspectFlags a, ImageAspectFlags b) {
        return static_cast<ImageAspectFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline ImageAspectFlags operator & (ImageAspectFlags a, ImageAspectFlags b) {
        return static_cast<ImageAspectFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class ImageUsageFlags : uint8_t {
        eNone = 0u,
        eTransfer_Src = 1u,
        eTransfer_Dst = 2u,
        eSampled = 4u,
        eStorage = 8u,
        eColor_Attachment = 16u,
        eDepth_Stencil_Attachment = 32u,
        eTransient_Attachment = 64u,
        eInput_Attachment = 128u,
    };
    inline ImageUsageFlags operator | (ImageUsageFlags a, ImageUsageFlags b) {
        return static_cast<ImageUsageFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline ImageUsageFlags operator & (ImageUsageFlags a, ImageUsageFlags b) {
        return static_cast<ImageUsageFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class ImageTiling : uint8_t {
        eOptimal = 0u,
        eLinear = 1u
    };

    enum class ImageCreateFlags : uint8_t {
        eNone = 0u,
        eMutable_Format = 1u,
        eCube_Compatible = 2u,
    };
    inline ImageCreateFlags operator | (ImageCreateFlags a, ImageCreateFlags b) {
        return static_cast<ImageCreateFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline ImageCreateFlags operator & (ImageCreateFlags a, ImageCreateFlags b) {
        return static_cast<ImageCreateFlags>(std::to_underlying(a) & std::to_underlying(b));
    }
    
    struct ImageInfo {
        ImageType type = ImageType::eType_2D;
        ImageUsageFlags usage = ImageUsageFlags::eNone;
        SharingMode sharing = SharingMode::eExclusive;
        Format format = Format::eR8G8B8A8_Unorm;
        ImageAspectFlags aspectFlags = ImageAspectFlags::eColor;
        ImageTiling tiling = ImageTiling::eOptimal;
        ImageCreateFlags flags = ImageCreateFlags::eNone;
        MemoryPropertyFlags memoryFlags = MemoryPropertyFlags::eDeviceLocal;
        MemoryExtendedFlags memoryExtFlags = MemoryExtendedFlags::eNone;
        Extent3D extent;
        uint8_t mipLevels = 1u;
        uint8_t arrayLayers = 1u;
        uint8_t samples = 1u;
        stl::vector<uint32_t> queueFamilyIndices = {};

        ImageInfo() = default;
        ImageInfo(ImageUsageFlags usage, Format format, Extent3D extent) : 
            usage(usage),
            format(format),
            extent(extent),
            aspectFlags(isDepthStencilFormat(format) ? ImageAspectFlags::eDepthStencil : 
                (isDepthFormat(format) ? ImageAspectFlags::eDepth: ImageAspectFlags::eColor)) {}

        ImageInfo(ImageUsageFlags usage, Format format, Extent3D extent, 
            ImageAspectFlags aspectFlags,
            ImageType type = ImageType::eType_2D,
            uint8_t mipLevels = 1u,
            uint8_t arrayLayers = 1u,
            uint8_t samples = 1u,
            ImageTiling tiling = ImageTiling::eOptimal,
            ImageCreateFlags flags = ImageCreateFlags::eNone,
            SharingMode sharing = SharingMode::eExclusive,
            MemoryPropertyFlags memoryFlags = MemoryPropertyFlags::eDeviceLocal,
            MemoryExtendedFlags memoryExtFlags = MemoryExtendedFlags::eNone,
            stl::vector<uint32_t> queueFamilyIndices = {}
        ) : 
            type(type),
            usage(usage),
            sharing(sharing),
            format(format),
            aspectFlags(aspectFlags),
            tiling(tiling),
            flags(flags),
            memoryFlags(memoryFlags),
            memoryExtFlags(memoryExtFlags),
            extent(extent),
            mipLevels(mipLevels),
            arrayLayers(arrayLayers),
            samples(samples),
            queueFamilyIndices(std::move(queueFamilyIndices)) {}        
    };

    struct ImageSubresourceLayers {
        ImageAspectFlags aspectMask = ImageAspectFlags::eColor;
        uint8_t mipLevel = 0u;
        uint8_t baseArrayLayer = 0u;
        uint8_t layerCount = 1u;
    };

    struct CopyBufferImageInfo {
        size_t bufferOffset = 0u;
        uint32_t bufferRowLength = 0u;
        uint32_t bufferImageHeight = 0u;
        ImageSubresourceLayers imageSubresource{};
        Offset3D imageOffset{};
        Extent3D imageExtent{};
    };

    struct CopyImage2ImageInfo {
        ImageSubresourceLayers srcSubresource{};
        ImageSubresourceLayers dstSubresource{};
        Offset3D srcOffset{};
        Offset3D dstOffset{};
        Extent3D extent{};
    };

    enum class BufferCreateFlags : uint8_t {
        eNone = 0u,
        eSparse_Binding = 1u,
        eSparse_Residency = 2u,
        eSparse_Aliased = 4u,
        eProtected = 8u
    };
    inline BufferCreateFlags operator | (BufferCreateFlags a, BufferCreateFlags b) {
        return static_cast<BufferCreateFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline BufferCreateFlags operator & (BufferCreateFlags a, BufferCreateFlags b) {
        return static_cast<BufferCreateFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    struct BufferInfo {
        BufferUsageFlags usage = BufferUsageFlags::eNone;
        MemoryPropertyFlags memoryFlags = MemoryPropertyFlags::eDeviceLocal;
        MemoryExtendedFlags memoryExtFlags = MemoryExtendedFlags::eNone;
        BufferCreateFlags flags = BufferCreateFlags::eNone;
        SharingMode sharing = SharingMode::eExclusive;
        stl::vector<uint32_t> queueFamilyIndices = {}; // It is ignored if sharingMode is not eConcurrent.

        BufferInfo(BufferUsageFlags usage, MemoryPropertyFlags memoryFlags, 
            MemoryExtendedFlags memoryExtFlags = MemoryExtendedFlags::eNone,
            BufferCreateFlags flags = BufferCreateFlags::eNone,
            SharingMode sharing = SharingMode::eExclusive,
            stl::vector<uint32_t> queueFamilyIndices = {}) : 
            usage(usage),
            memoryFlags(memoryFlags),
            memoryExtFlags(memoryExtFlags),
            flags(flags),
            sharing(sharing),
            queueFamilyIndices(std::move(queueFamilyIndices)) {}
    };

    enum class EventCreateFlags : uint8_t {
        eNone = 0u,
        eDeviceOnly = 1u
    };
    inline EventCreateFlags operator | (EventCreateFlags a, EventCreateFlags b) {
        return static_cast<EventCreateFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline EventCreateFlags operator & (EventCreateFlags a, EventCreateFlags b) {
        return static_cast<EventCreateFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class ResolveModeFlags : uint8_t {
        eNone = 0u,
        eSample_Zero = 1u,
        eAverage = 2u,
        eMin = 4u,
        eMax = 8u,
    };
    inline ResolveModeFlags operator | (ResolveModeFlags a, ResolveModeFlags b) {
        return static_cast<ResolveModeFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline ResolveModeFlags operator & (ResolveModeFlags a, ResolveModeFlags b) {
        return static_cast<ResolveModeFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class AttachmentLoadOp : uint8_t {
        eLoad = 0u,
        eClear = 1u,
        eDont_Care = 2u,
        eNone = 3u
    };

    enum class AttachmentStoreOp : uint8_t {
        eStore = 0u,
        eDont_Care = 1u,
        eNone = 2u
    };

    struct AttachmentLoadStoreOp {
        uint8_t mask = 0u;
        
        constexpr AttachmentLoadStoreOp() : 
            mask(std::to_underlying(AttachmentLoadOp::eDont_Care) | (std::to_underlying(AttachmentStoreOp::eDont_Care) << 4u)) {}

        constexpr AttachmentLoadStoreOp(AttachmentLoadOp loadOp, AttachmentStoreOp storeOp) : 
            mask(std::to_underlying(loadOp) | (std::to_underlying(storeOp) << 4u)) {}

        inline AttachmentLoadOp loadOp() const noexcept {
            return static_cast<AttachmentLoadOp>(mask & 0b0011u);
        }

        inline AttachmentStoreOp storeOp() const noexcept {
            return static_cast<AttachmentStoreOp>((mask >> 4u) & 0b0011u);
        }
    };

    enum class ImageLayout : uint8_t {
        eUndefined = 0u,
        eGeneral = 1u,
        eColor_Attachment_Optimal = 2u,
        eDepth_Stencil_Attachment_Optimal = 3u,
        eDepth_Stencil_Read_Only_Optimal = 4u,
        eShader_Read_Only_Optimal = 5u,
        eTransfer_Src_Optimal = 6u,
        eTransfer_Dst_Optimal = 7u,
        ePreinitialized = 8u,
        eDepth_Read_Only_Stencil_Attachment_Optimal = 9u,
        eDepth_Attachment_Stencil_Read_Only_Optimal = 10u,
        eDepth_Attachment_Optimal = 11u,
        eDepth_Read_Only_Optimal = 12u,
        eStencil_Attachment_Optimal = 13u,
        eStencil_Read_Only_Optimal = 14u,
        eRead_Only_Optimal = 15u,
        eAttachment_Optimal = 16u,
        ePresent_Src = 17u,
        eShared_Present = 18u,
        eFragment_Density_Map_Optimal_Ext = 19u,
        eFragment_Shading_Rate_Attachment_Optimal = 20u,
        eAttachment_Feedback_Loop_Optimal_Ext = 21u,
    };

    enum class FilterMode : uint8_t {
        eNearest = 0u,
        eLinear = 1u
    };

    struct ImageBlit {
        ImageSubresourceLayers srcSubresource;
        stl::array<Offset3D, 2u> srcOffsets;
        ImageSubresourceLayers dstSubresource;
        stl::array<Offset3D, 2u> dstOffsets;
    };

    struct BlitImageInfo {
        ImageLayout srcImageLayout = ImageLayout::eTransfer_Src_Optimal;
        ImageLayout dstImageLayout = ImageLayout::eTransfer_Dst_Optimal;
        stl::array<ImageBlit, 8u> regions;
        uint8_t regionsCount = 0u;
        FilterMode filter = FilterMode::eLinear;
    };

    struct AttachmentDescription {
        Format format = Format::eR8G8B8A8_Unorm;
        AttachmentLoadStoreOp loadStoreOp{};
        AttachmentLoadStoreOp stencilLoadStoreOp{};
        ImageLayout initialLayout = ImageLayout::eUndefined;
        ImageLayout finalLayout = ImageLayout::eColor_Attachment_Optimal;
        uint8_t samples = 1u;
        bool aliasFlag = false;
    };

    struct AttachmentReference {
        stl::pair<const void*, size_t> pNext = {nullptr, 0u}; // value + hash(value) 
        uint16_t attachment = 0xff'ffu; // undefined
        ImageLayout layout = ImageLayout::eUndefined;
        ImageAspectFlags aspectMask = ImageAspectFlags::eNone;

        void setNext(void * next, size_t hash) noexcept {
            pNext = {next, hash};
        }

        inline bool operator == (const AttachmentReference & other) const noexcept {
            return attachment == other.attachment && layout == other.layout && aspectMask == other.aspectMask &&
            pNext.second == other.pNext.second;
        };
    };

    enum class PipelineStageFlags : uint32_t {
        eNone = 0u,
        eTop_Of_Pipe = 0x00000001u,
        eDraw_Indirect = 0x00000002u,
        eVertex_Input = 0x00000004u,
        eVertex_Shader = 0x00000008u,
        eTesselation_Control_Shader = 0x00000010u,
        eTesselation_Evaluation_Shader = 0x00000020u,
        eGeometry_Shader = 0x00000040u,
        eFragment_Shader = 0x00000080u,
        eEarly_Fragment_Tests = 0x00000100u,
        eLate_Fragment_Tests = 0x00000200u,
        eColor_Attachment_Output = 0x00000400u,
        eCompute_Shader = 0x00000800u,
        eTransfer = 0x00001000u,
        eBottom_Of_Pipe = 0x00002000u,
        eHost = 0x00004000u,
        eAll_Graphics = 0x00008000u,
        eAll_Commands = 0x00010000u,
        eTransform_Feedback_Ext = 0x01000000u,
        eConditional_Rendering_Ext = 0x00040000u,
        eAcceleration_Structure_Build = 0x02000000u,
        eRay_Tracing_Shader = 0x00200000u,
        eFragment_Density_Process_Ext = 0x00800000u,
        eFragment_Shading_Rate_Attachment = 0x00400000u,
        eCommand_Preprocess_Ext = 0x00020000u,
        eTast_Shader_Ext = 0x00080000u,
        eMesh_Shader_Ext = 0x00100000u,
    };
    inline PipelineStageFlags operator | (PipelineStageFlags a, PipelineStageFlags b) {
        return static_cast<PipelineStageFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline PipelineStageFlags operator & (PipelineStageFlags a, PipelineStageFlags b) {
        return static_cast<PipelineStageFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class AccessFlags : uint32_t {
        eNone = 0u,
        eIndirect_Command_Read = 0x00000001u,
        eIndex_Read = 0x00000002u,
        eVertex_Attribute_Read = 0x00000004u,
        eUniform_Read = 0x00000008u,
        eInput_Attachment_Read = 0x00000010u,
        eShader_Read = 0x00000020u,
        eShader_Write = 0x00000040u,
        eColor_Attachment_Read = 0x00000080u,
        eColor_Attachment_Write= 0x00000100u,
        eDepth_Stencil_Attachment_Read = 0x00000200u,
        eDepth_Stencil_Attachment_Write = 0x00000400u,
        eTransfer_Read = 0x00000800u,
        eTransfer_Write = 0x00001000u,
        eHost_Read = 0x00002000u,
        eHost_Write = 0x00004000u,
        eMemory_Read = 0x00008000u,
        eMemory_Write = 0x00010000u,
        eTransfrom_Feedback_Write_Ext = 0x02000000u,
        eTransfrom_Feedback_Counter_Read_Ext = 0x04000000u,
        eTransfrom_Feedback_Counter_Write_Ext = 0x08000000u,
        eConditional_Rendering_Read_Ext = 0x00100000u,
        eColor_Attachment_Read_Noncoherent_Ext = 0x00080000u,
        eAcceleration_Structure_Read = 0x00200000u,
        eAcceleration_Structure_Write = 0x00400000u,
        eFragment_Density_Map_Read_Ext = 0x01000000u,
        eFragment_Shading_Rate_Attachment_Read = 0x00800000u,
        eCommand_Preprocess_Read_Ext = 0x00020000u,
        eCommand_Preprocess_Write_Ext = 0x00040000u,
    };
    inline AccessFlags operator | (AccessFlags a, AccessFlags b) {
        return static_cast<AccessFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline AccessFlags operator & (AccessFlags a, AccessFlags b) {
        return static_cast<AccessFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class DependencyFlags : uint8_t {
        eNone = 0u,
        eBy_Region = 0x00000001u,
        eDevice_Group = 0x00000004u,
        eView_Local = 0x00000002u,
        eFeedback_Loop_Ext = 0x00000008u,
        eQueue_Family_Ownership_Transfer_Use_All_Stages = 0x00000020u,
    };
    inline DependencyFlags operator | (DependencyFlags a, DependencyFlags b) {
        return static_cast<DependencyFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline DependencyFlags operator & (DependencyFlags a, DependencyFlags b) {
        return static_cast<DependencyFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class CommandPoolCreateFlags : uint8_t {
        eNone = 0u,
        eTransient = 1u,
        eReset_Command_Buffer = 2u,
        eProtected = 4u,
    };
    inline CommandPoolCreateFlags operator | (CommandPoolCreateFlags a, CommandPoolCreateFlags b) {
        return static_cast<CommandPoolCreateFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline CommandPoolCreateFlags operator & (CommandPoolCreateFlags a, CommandPoolCreateFlags b) {
        return static_cast<CommandPoolCreateFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class CommandBufferLevel : uint8_t {
        ePrimary = 0u,
        eSecondary = 1u,
    };

    enum class CommandBufferUsageFlags : uint8_t {
        eNone = 0u,
        eOne_Time_Submit= 1u,
        eRender_pass_Continue = 2u,
        eSimultaneous_Use = 4u,
    };
    inline CommandBufferUsageFlags operator | (CommandBufferUsageFlags a, CommandBufferUsageFlags b) {
        return static_cast<CommandBufferUsageFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline CommandBufferUsageFlags operator & (CommandBufferUsageFlags a, CommandBufferUsageFlags b) {
        return static_cast<CommandBufferUsageFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class QueryResultFlags : uint8_t {
        eNone = 0u,
        e64 = 1u,
        eWait = 2u,
        eWith_Availability= 4u,
        ePartial = 8u,
        eWith_Status = 16u,
    };
    inline QueryResultFlags operator | (QueryResultFlags a, QueryResultFlags b) {
        return static_cast<QueryResultFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline QueryResultFlags operator & (QueryResultFlags a, QueryResultFlags b) {
        return static_cast<QueryResultFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class QueryType : uint8_t {
        eOcclusion = 0u,
        ePipeline_Statistics = 1u,
        eTimestamp = 2u,
        eResult_Status_Only = 3u,
        eTransform_Feedback_Stream = 4u,
        ePerfomance_Query = 5u,
        eAcceleration_Structure_Compacted_Size = 6u,
        eAcceleration_Structure_Serialization_Size = 7u,
        eAcceleration_Structure_Compacted_Size_NV = 8u,
        ePerfomance_Query_INTEL = 9u,
        eMesh_Primitives_Generated = 10u,
        ePrimitives_Generated = 11u,
        eAcceleration_Structure_Serialization_Bottom_Level_Pointers = 12u,
        eAcceleration_Structure_Size = 13u,
        eMicromap_Serialization_Size = 14u,
        eMicromap_Compacted_Size = 15u
    };

    enum class QueryPipelineStatisticFlags : uint16_t {
        eNone = 0u,
        eInputAssemblyVertices = 0x00000001u,
        eInputAssemblyPrimitives = 0x00000002u,
        eVertexShaderInvocations = 0x00000004u,
        eGeometryShaderInvocations = 0x00000008u,
        eGeometryShaderPrimitives = 0x00000010u,
        eClippingInvocations = 0x00000020u,
        eClippingPrimitives = 0x00000040u,
        eFragmentShaderInvocations = 0x00000080u,
        eTessellationControlShaderPatches = 0x00000100u,
        eTessellationEvaluationShaderInvocations = 0x00000200u,
        eComputeShaderInvocations = 0x00000400u,
        eTaskShaderInvocationsEXT = 0x00000800u,
        eMeshShaderInvocationsEXT = 0x00001000u
    };
    inline QueryPipelineStatisticFlags operator | (QueryPipelineStatisticFlags a, QueryPipelineStatisticFlags b) {
        return static_cast<QueryPipelineStatisticFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline QueryPipelineStatisticFlags operator & (QueryPipelineStatisticFlags a, QueryPipelineStatisticFlags b) {
        return static_cast<QueryPipelineStatisticFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class QueryControlFlags : uint8_t {
        eNone = 0u,
        ePrecise = 0x00000001u
    };
    inline QueryControlFlags operator | (QueryControlFlags a, QueryControlFlags b) {
        return static_cast<QueryControlFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline QueryControlFlags operator & (QueryControlFlags a, QueryControlFlags b) {
        return static_cast<QueryControlFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    struct QueryPoolInfo {
        QueryType queryType;
        uint32_t  queryCount;
        QueryPipelineStatisticFlags pipelineStatistics = QueryPipelineStatisticFlags::eNone;
    };

    enum class CommandBufferResetFlags : uint8_t {
        eNone = 0u,
        eRelease_Resources = 1u
    };
    inline CommandBufferResetFlags operator | (CommandBufferResetFlags a, CommandBufferResetFlags b) {
        return static_cast<CommandBufferResetFlags>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline CommandBufferResetFlags operator & (CommandBufferResetFlags a, CommandBufferResetFlags b) {
        return static_cast<CommandBufferResetFlags>(std::to_underlying(a) & std::to_underlying(b));
    }

    enum class FenceCreateFlags : uint8_t {
        eUnsignaled = 0u,
        eSignaled = 1u
    };

    enum class SubpassContents : uint8_t {
        ePrimaryBufferInline = 0u,
        eSecondaryBuffers = 1u
    };

    enum class IndexType : uint8_t {
        eU16 = 0u,
        eU32 = 1u,
        eU8 = 2u, // index_type_uint8
    };

    enum class VertexInputRate : uint8_t {
        eVertex = 0u,
        eInstance = 1u,
    };

    enum class PipelineBindPoint : uint8_t {
        eGraphics = 0u,
        eCompute = 1u,
        eRay_Tracing = 2u
    };

    enum class CompareOp : uint8_t {
        eNever = 0u,
        eLess = 1u,
        eEqual = 2u,
        eLequal = 3u,
        eGreater = 4u,
        eNot_Equal = 5u,
        eGequal = 6u,
        eAlways = 7u,
    };

    enum class StencilOp : uint8_t {
        eKeep = 0u,
        eZero = 1u,
        eReplace = 2u,
        eInc_And_Clamp = 3u,
        eDec_And_Clamp = 4u,
        eInvert = 5u,
        eInc_And_Wrap = 6u,
        eDec_And_Wrap = 7u,
    };

    enum class BlendOp : uint8_t { // 3 bits
        eAdd = 0u,
        eSubtract = 1u,
        eReverse_Subtract = 2u,
        eMin = 3u,
        eMax = 4u,
    };

    enum class BlendFactor : uint8_t { // 4 bits
        eZero = 0u,
        eOne = 1u,
        eSrc_Color = 2u,
        eOne_Minus_Src_Color = 3u,
        eDst_Color = 4u,
        eOne_Minus_Dst_Color = 5u,
        eSrc_Alpha = 6u,
        eOne_Minus_Src_Alpha = 7u,
        eDst_Alpha = 8u,
        eOne_Minus_Dst_Alpha = 9u,
        eConst_Color = 10u,
        eOne_Minus_Const_Color = 11u,
        eConst_Alpha = 12u,
        eOne_Minus_Const_Alpha = 13u,
        eSrc_Alpha_Saturate = 14u,
    };

    enum class PrimitiveTopology : uint8_t {
        ePoints = 0u,
        eLines = 1u,
        eLine_Strip = 2u,
        eTriangles = 3u,
        eTriangle_Strip = 4u,
        eTriangle_Fan = 5u,
        eLines_With_Adjacency = 6u,
        eLine_Strip_With_Adjacency = 7u,
        eTriangles_With_Adjacency = 8u,
        eTriangle_Strip_With_Adjacency = 9u,
        ePatchs = 10u,
    };

    enum class PolygonMode : uint8_t {
        eFill = 0u,
        eLine = 1u,
        ePoint = 2u,
    };

    enum class FrontFace : uint8_t {
        eCounter_Clockwise = 0u,
        eClockwise = 1u,
    };

    enum class CullMode: uint8_t {
        eNone = 0u,
        eFront = 1u,
        eBack = 2u,
        eFront_And_Back = 3u,
    };

    struct StencilOpState {
        uint32_t compareMask = 0u;
        uint32_t writeMask = 0u;
        uint32_t reference = 0u;
        StencilOp failOp{};
        StencilOp passOp{};
        StencilOp depthFailOp{};
        CompareOp compareOp{};
    };

    struct DepthState {
        bool depthTestEnable = false;
        bool depthWriteEnable = false;
        CompareOp depthCompareOp = CompareOp::eAlways;
        bool depthBoundsTestEnable = false;
        float minDepthBounds = 0.0f;
        float maxDepthBounds = 1.0f;
    };

    struct StencilState {
        StencilOpState front{};
        StencilOpState back{};
        bool stencilTestEnable = false;
    };

    struct BlendMode {
        bool blendEnable = false;                           // 1bit
        BlendFactor srcColorFactor = BlendFactor::eZero;    // 4bits
        BlendFactor dstColorFactor = BlendFactor::eZero;    // 4bits
        BlendOp colorBlendOp = BlendOp::eAdd;               // 3bits
        BlendFactor srcAlphaFactor = BlendFactor::eZero;    // 4bits
        BlendFactor dstAlphaFactor = BlendFactor::eZero;    // 4bits
        BlendOp alphaBlendOp = BlendOp::eAdd;               // 3bits
        ColorComponent colorWriteMask = ColorComponent::eR | ColorComponent::eG | ColorComponent::eB | ColorComponent::eA;  // 4bits

        constexpr BlendMode(
            bool blendEnable,
            BlendFactor srcColorFactor = BlendFactor::eZero,
            BlendFactor dstColorFactor = BlendFactor::eZero,
            BlendFactor srcAlphaFactor = BlendFactor::eZero,
            BlendFactor dstAlphaFactor = BlendFactor::eZero,
            BlendOp colorBlendOp = BlendOp::eAdd,
            BlendOp alphaBlendOp = BlendOp::eAdd,
            ColorComponent colorWriteMask = ColorComponent::eR | ColorComponent::eG | ColorComponent::eB | ColorComponent::eA
        ) :
            blendEnable(blendEnable),
            srcColorFactor(srcColorFactor),
            dstColorFactor(dstColorFactor),
            colorBlendOp(colorBlendOp),
            srcAlphaFactor(srcAlphaFactor),
            dstAlphaFactor(dstAlphaFactor),
            alphaBlendOp(alphaBlendOp),
            colorWriteMask(colorWriteMask) {
        }

        constexpr BlendMode(uint32_t blendValue) : 
            blendEnable((blendValue >> 0u) & 0b0001u),
            srcColorFactor(static_cast<BlendFactor>((blendValue >> 1u) & 0b1111u)),
            dstColorFactor(static_cast<BlendFactor>((blendValue >> 5u) & 0b1111u)),
            colorBlendOp(static_cast<BlendOp>((blendValue >> 9u) & 0b0111u)),
            srcAlphaFactor(static_cast<BlendFactor>((blendValue >> 12u) & 0b1111u)),
            dstAlphaFactor(static_cast<BlendFactor>((blendValue >> 16u) & 0b1111u)),
            alphaBlendOp(static_cast<BlendOp>((blendValue >> 20u) & 0b0111u)),
            colorWriteMask(static_cast<ColorComponent>((blendValue >> 23u) & 0b1111u)) {
        }

        constexpr uint32_t asNumber() const noexcept {
            if (!blendEnable) return static_cast<uint32_t>(colorWriteMask) << 23u;

            return static_cast<uint32_t>(blendEnable) << 0u | // + 1u
                   static_cast<uint32_t>(srcColorFactor) << 1u | // + 4u
                   static_cast<uint32_t>(dstColorFactor) << 5u | // + 4u
                   static_cast<uint32_t>(colorBlendOp) << 9u | // + 3u
                   static_cast<uint32_t>(srcAlphaFactor) << 12u | // + 4u
                   static_cast<uint32_t>(dstAlphaFactor) << 16u | // + 4u
                   static_cast<uint32_t>(alphaBlendOp) << 20u | // + 3u
                   static_cast<uint32_t>(colorWriteMask) << 23u; // + 4u
        }
    }; // 4bits * 5 + 3bits * 2 + 1bit = 27bit

    using EncodedBlendMode = uint32_t;

    namespace common_blend_modes {
        // normal modes
        inline constexpr static EncodedBlendMode blend_none	    = BlendMode(false).asNumber();
        inline constexpr static EncodedBlendMode blend_alpha	= BlendMode(
            true, BlendFactor::eSrc_Alpha, 
            BlendFactor::eOne_Minus_Src_Alpha, 
            BlendFactor::eSrc_Alpha, 
            BlendFactor::eOne_Minus_Src_Alpha).asNumber();

        inline constexpr static EncodedBlendMode blend_add	= BlendMode(
            true, BlendFactor::eSrc_Alpha, 
            BlendFactor::eOne, 
            BlendFactor::eSrc_Alpha, 
            BlendFactor::eOne).asNumber();

        inline constexpr static EncodedBlendMode blend_multiply	= BlendMode(
            true, BlendFactor::eDst_Color, 
            BlendFactor::eZero, 
            BlendFactor::eDst_Color, 
            BlendFactor::eZero).asNumber();
        // premultiply alpha modes
        // https://gamedev.ru/code/articles/PremultipliedAlpha
        // https://habr.com/ru/company/playrix/blog/336304/
        // https://apoorvaj.io/alpha-compositing-opengl-blending-and-premultiplied-alpha/
        inline constexpr static EncodedBlendMode blend_alpha_pma	= BlendMode(
            true, BlendFactor::eOne, 
            BlendFactor::eOne_Minus_Src_Alpha, 
            BlendFactor::eOne, 
            BlendFactor::eOne_Minus_Src_Alpha).asNumber();

        inline constexpr static EncodedBlendMode blend_add_pma	= BlendMode(
            true, BlendFactor::eOne, 
            BlendFactor::eOne, 
            BlendFactor::eOne, 
            BlendFactor::eOne).asNumber();    
	};

    inline constexpr uint8_t kDynamicStatesCount = 24u;
    enum class DynamicState : uint32_t {
        eNone = 0u,
        eViewport = 1u << 0u,
        eScissor = 1u << 1u,
        eLine_Width = 1u << 2u,
        eDepth_Bias = 1u << 3u,
        eBlend_Constants = 1u << 4u,
        eDepth_Bounds = 1u << 5u,
        eStencil_Compare_Mask = 1u << 6u,
        eStencil_Write_Mask = 1u << 7u,
        eStencil_Refference = 1u << 8u,
        eCull_Mode = 1u << 9u,
        eFront_Face = 1u << 10u,
        ePrimitive_Topology = 1u << 11u,
        eViewport_With_Count = 1u << 12u,
        eScissor_With_Count = 1u << 13u,
        eVertex_Input_Binding_Stride = 1u << 14u,
        eDepth_Test_Enable = 1u << 15u,
        eDepth_Write_Enable = 1u << 16u,
        eDepth_Compare_Op = 1u << 17u,
        eDepth_Bounds_Test_Enable = 1u << 18u,
        eStencil_Test_Enable = 1u << 19u,
        eStencil_Op = 1u << 20u,
        eRasterizer_Discard_Enable = 1u << 21u,
        eDepth_Bias_Enable = 1u << 22u,
        ePrimitive_Restart_Enable = 1u << 23u,
        eAll = 0x7f'ff'ff'ffu
    };
    inline DynamicState operator | (DynamicState a, DynamicState b) {
        return static_cast<DynamicState>(std::to_underlying(a) | std::to_underlying(b));
    }
    inline DynamicState operator & (DynamicState a, DynamicState b) {
        return static_cast<DynamicState>(std::to_underlying(a) & std::to_underlying(b));
    }

    struct VertexInputBinding {
        uint16_t stride = 0u;
        uint8_t binding = 0u;
        VertexInputRate inputRate = VertexInputRate::eVertex;

        template <typename T>
        static VertexInputBinding create(uint8_t binding, VertexInputRate inputRate = VertexInputRate::eVertex) noexcept {
            return {
                .stride = sizeof(T),
                .binding = binding, 
                .inputRate = inputRate 
            };
        }
    };

    enum class SamplerAddressMode : uint8_t {
        eRepeat = 0u,
        eMirrored_Repeat = 1u,
        eClamp_To_Edge = 2u,
        eClamp_To_Border = 3u
    };

    enum class BorderColor: uint8_t {
        eFloat_Transparent_Black = 0,
        eInt_Transparent_Black = 1,
        eFloat_Opaque_Black = 2,
        eInt_Opaque_Black = 3,
        eFloat_Opaque_White = 4,
        eInt_Opaque_White = 5
    };

    struct SamplerInfo {
        FilterMode magFilter = FilterMode::eNearest;
        FilterMode minFilter = FilterMode::eNearest;
        FilterMode mipMapFilter = FilterMode::eNearest;
        SamplerAddressMode addressModeU = SamplerAddressMode::eRepeat;
        SamplerAddressMode addressModeV = SamplerAddressMode::eRepeat;
        SamplerAddressMode addressModeW = SamplerAddressMode::eRepeat;
        CompareOp compareOp = CompareOp::eNever;
        BorderColor borderColor = BorderColor::eFloat_Opaque_Black;
        float maxAnisotropy = 0.0f;

        inline size_t hash() const noexcept {
            constexpr auto kAnisotropyRound = 100;
            const auto value = (static_cast<uint64_t>(magFilter) << 0u) | 
                               (static_cast<uint64_t>(magFilter) << 8u) |
                               (static_cast<uint64_t>(mipMapFilter) << 16u) |
                               (static_cast<uint64_t>(addressModeU) << 24u) |
                               (static_cast<uint64_t>(addressModeV) << 32u) |
                               (static_cast<uint64_t>(addressModeW) << 40u) |
                               (static_cast<uint64_t>(compareOp) << 48u) |
                               (static_cast<uint64_t>(borderColor) << 56u);

            return hash::hash_combine(
                value,
                std::round(maxAnisotropy * kAnisotropyRound)
            );
        }

        inline bool operator == (const SamplerInfo & other) const noexcept = default;
    };

    struct SamplerValue {
        SamplerInfo info{};
        void * evaluatedSampler = nullptr;
    };

    struct VertexInputAttribute {
        uint8_t location = 0u;
        uint8_t binding = 0u;
        Format format = Format::eUndefined;
        uint32_t offset = 0u;
    };

    namespace shaders {
        enum class DescriptorType : uint16_t {
            eSampler = 0u,
            eCombined_Image_Sampler = 1u,
            eSampled_Image = 2u,
            eStorage_Image = 3u,
            eUniform_Texel_Buffer = 4u,
            eStorage_Texel_Buffer = 5u,
            eUniform_Buffer = 6u,
            eStorage_Buffer = 7u,
            eUniform_Buffer_Dynamic = 8u,
            eStorage_Buffer_Dynamic = 9u,
            eInput_Attachment = 10u,
            eInline_Uniform_Block = 11u,
            eAcceleretion_Structure = 12u,
            eCount = 13u
        };

        using ResourceType = DescriptorType;

        enum class DescriptorPoolFlags : uint8_t {
            eNone = 0u, 
            eFree_Descriptor_Set = 1u,
            eUpdate_After_Bind = 2u
        };
        inline DescriptorPoolFlags operator | (DescriptorPoolFlags a, DescriptorPoolFlags b) {
            return static_cast<DescriptorPoolFlags>(std::to_underlying(a) | std::to_underlying(b));
        }
        inline DescriptorPoolFlags operator & (DescriptorPoolFlags a, DescriptorPoolFlags b) {
            return static_cast<DescriptorPoolFlags>(std::to_underlying(a) & std::to_underlying(b));
        }

        enum class DescriptorSetLayoutFlags : uint8_t {
            eNone = 0u,
            eUpdate_After_Bind_Pool = 1u,
            ePush_Descriptor = 2u,
            eDescriptor_Buffer = 3u, // _descriptor_buffer
            eEmbedded_Immutable_Samplers = 4u, // _descriptor_buffer
        };
        inline DescriptorSetLayoutFlags operator | (DescriptorSetLayoutFlags a, DescriptorSetLayoutFlags b) {
            return static_cast<DescriptorSetLayoutFlags>(std::to_underlying(a) | std::to_underlying(b));
        }
        inline DescriptorSetLayoutFlags operator & (DescriptorSetLayoutFlags a, DescriptorSetLayoutFlags b) {
            return static_cast<DescriptorSetLayoutFlags>(std::to_underlying(a) & std::to_underlying(b));
        }

        enum class Stages : uint16_t {
            eNone = 0u,
            eVertex = 1u,
            eTesselation_Control = 2u,
            eTesselation_Evaluation = 4u,
            eGeometry = 8u,
            eFragment = 16u,
            eCompute = 32u,
            eRaygen = 64u,
            eAny_Hit = 128u,        // ray_tracing_pipeline
            eClosest_Hit = 256u,    // ray_tracing_pipeline
            eMiss = 512u,           // ray_tracing_pipeline
            eIntersection = 1024u,  // ray_tracing_pipeline
            eCallable = 2048u,      // ray_tracing_pipeline
            eTask = 4096u,          // mesh_shader
            eMesh = 8192u,          // mesh_shader
        };
        inline Stages operator | (Stages a, Stages b) {
            return static_cast<Stages>(std::to_underlying(a) | std::to_underlying(b));
        }
        inline Stages operator & (Stages a, Stages b) {
            return static_cast<Stages>(std::to_underlying(a) & std::to_underlying(b));
        }
        inline constexpr uint8_t kStagesCount = 15u;

        struct BindingLayout {
            uint16_t binding = 0xff'ffu;
            DescriptorType type = DescriptorType::eCount;
            Stages shaderStages = Stages::eNone;
            uint16_t count = 1u;
            
            inline uint64_t hash64() const noexcept {
                return static_cast<uint64_t>(shaderStages) << 48u |
                       static_cast<uint64_t>(count) << 32u |
                       static_cast<uint64_t>(type) << 16u |
                       static_cast<uint64_t>(binding) << 0u;
            }

            inline size_t hash() const noexcept { return static_cast<size_t>(hash64()); }
            inline bool operator == (const BindingLayout & other) const noexcept = default;
        };

        struct BindingSetLayout {
            inline static constexpr uint32_t kMaxBindingsInSetOnStack = 8u; 
            // BindingSetLayout() = default;
            explicit BindingSetLayout(dynamic_array<BindingLayout, kMaxBindingsInSetOnStack> && l) : layouts(std::move(l)) {
                auto view = layouts.view();
                stl::sort(view.begin(), view.end(), [](const auto & l0, const auto & l1){ return l0.binding < l1.binding; });

                for (const auto & layout : view) {
                    hash::hash_combo(hashValue, layout.hash());
                }
                hash::hash_combo(hashValue, std::to_underlying(flags));
            }

            inline size_t hash() const noexcept {
                return hashValue;
            }

            inline bool operator == (const BindingSetLayout & other) const noexcept = default;

            const BindingLayout * findLayout(uint32_t binding) const noexcept {
                auto const view = layouts.view();
                if (auto it = std::find_if(view.begin(), view.end(), [binding](const BindingLayout & l) noexcept {
                    return l.binding == binding;
                }); it != view.end()) {
                    return &(*it);
                }
                return nullptr;
            }

            BindingLayout * findLayout(uint32_t binding) noexcept {
                auto const view = layouts.view();
                if (auto it = std::find_if(view.begin(), view.end(), [binding](const BindingLayout & l) noexcept {
                    return l.binding == binding;
                }); it != view.end()) {
                    return &(*it);
                }
                return nullptr;
            }

            dynamic_array<BindingLayout, kMaxBindingsInSetOnStack> layouts;
            DescriptorSetLayoutFlags flags = DescriptorSetLayoutFlags::eNone;
            size_t hashValue = 0u;
        };

        struct SpecializationInfo {
            uint32_t dynamicDescriptorsMask = 0u; 
            // to be continued...
        };
    }
    
}