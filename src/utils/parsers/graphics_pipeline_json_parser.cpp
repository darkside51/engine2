#include "graphics_pipeline_json_parser.h"

#include <string_view>

namespace engine {

    rhi::Format parseFormat(std::string_view format) noexcept {
        if (format == "eUndefined") return rhi::Format::eUndefined;
        if (format == "eR4G4_Unorm_Pack8") return rhi::Format::eR4G4_Unorm_Pack8;
        if (format == "eR4G4B4A4_Unorm_Pack16") return rhi::Format::eR4G4B4A4_Unorm_Pack16;
        if (format == "eB4G4R4A4_Unorm_Pack16") return rhi::Format::eB4G4R4A4_Unorm_Pack16;
        if (format == "eR5G6B5_Unorm_Pack16") return rhi::Format::eR5G6B5_Unorm_Pack16;
        if (format == "eB5G6R5_Unorm_Pack16") return rhi::Format::eB5G6R5_Unorm_Pack16;
        if (format == "eR5G5B5A1_Unorm_Pack16") return rhi::Format::eR5G5B5A1_Unorm_Pack16;
        if (format == "eB5G5R5A1_Unorm_Pack16") return rhi::Format::eB5G5R5A1_Unorm_Pack16;
        if (format == "eA1R5G5B5_Unorm_Pack16") return rhi::Format::eA1R5G5B5_Unorm_Pack16;

        if (format == "eR8_Unorm") return rhi::Format::eR8_Unorm;
        if (format == "eR8_Snorm") return rhi::Format::eR8_Snorm;
        if (format == "eR8_Uscaled") return rhi::Format::eR8_Uscaled;
        if (format == "eR8_Sscaled") return rhi::Format::eR8_Sscaled;
        if (format == "eR8_Uint") return rhi::Format::eR8_Uint;
        if (format == "eR8_Sint") return rhi::Format::eR8_Sint;
        if (format == "eR8_Srgb") return rhi::Format::eR8_Srgb;
        if (format == "eR8G8_Unorm") return rhi::Format::eR8G8_Unorm;
        if (format == "eR8G8_Snorm") return rhi::Format::eR8G8_Snorm;
        if (format == "eR8G8_Uscaled") return rhi::Format::eR8G8_Uscaled;
        if (format == "eR8G8_Sscaled") return rhi::Format::eR8G8_Sscaled;
        if (format == "eR8G8_Uint") return rhi::Format::eR8G8_Uint;
        if (format == "eR8G8_Sint") return rhi::Format::eR8G8_Sint;
        if (format == "eR8G8_Srgb") return rhi::Format::eR8G8_Srgb;
        if (format == "eR8G8B8_Unorm") return rhi::Format::eR8G8B8_Unorm;
        if (format == "eR8G8B8_Snorm") return rhi::Format::eR8G8B8_Snorm;
        if (format == "eR8G8B8_Uscaled") return rhi::Format::eR8G8B8_Uscaled;
        if (format == "eR8G8B8_Sscaled") return rhi::Format::eR8G8B8_Sscaled;
        if (format == "eR8G8B8_Uint") return rhi::Format::eR8G8B8_Uint;
        if (format == "eR8G8B8_Sint") return rhi::Format::eR8G8B8_Sint;
        if (format == "eR8G8B8_Srgb") return rhi::Format::eR8G8B8_Srgb;
        if (format == "eB8G8R8_Unorm") return rhi::Format::eB8G8R8_Unorm;
        if (format == "eB8G8R8_Snorm") return rhi::Format::eB8G8R8_Snorm;
        if (format == "eB8G8R8_Uscaled") return rhi::Format::eB8G8R8_Uscaled;
        if (format == "eB8G8R8_Sscaled") return rhi::Format::eB8G8R8_Sscaled;
        if (format == "eB8G8R8_Uint") return rhi::Format::eB8G8R8_Uint;
        if (format == "eB8G8R8_Sint") return rhi::Format::eB8G8R8_Sint;
        if (format == "eB8G8R8_Srgb") return rhi::Format::eB8G8R8_Srgb;
        if (format == "eR8G8B8A8_Unorm") return rhi::Format::eR8G8B8A8_Unorm;
        if (format == "eR8G8B8A8_Snorm") return rhi::Format::eR8G8B8A8_Snorm;
        if (format == "eR8G8B8A8_Uscaled") return rhi::Format::eR8G8B8A8_Uscaled;
        if (format == "eR8G8B8A8_Sscaled") return rhi::Format::eR8G8B8A8_Sscaled;
        if (format == "eR8G8B8A8_Uint") return rhi::Format::eR8G8B8A8_Uint;
        if (format == "eR8G8B8A8_Sint") return rhi::Format::eR8G8B8A8_Sint;
        if (format == "eR8G8B8A8_Srgb") return rhi::Format::eR8G8B8A8_Srgb;
        if (format == "eB8G8R8A8_Unorm") return rhi::Format::eB8G8R8A8_Unorm;
        if (format == "eB8G8R8A8_Snorm") return rhi::Format::eB8G8R8A8_Snorm;
        if (format == "eB8G8R8A8_Uscaled") return rhi::Format::eB8G8R8A8_Uscaled;
        if (format == "eB8G8R8A8_Sscaled") return rhi::Format::eB8G8R8A8_Sscaled;
        if (format == "eB8G8R8A8_Uint") return rhi::Format::eB8G8R8A8_Uint;
        if (format == "eB8G8R8A8_Sint") return rhi::Format::eB8G8R8A8_Sint;
        if (format == "eB8G8R8A8_Srgb") return rhi::Format::eB8G8R8A8_Srgb;
        if (format == "eA8B8G8R8_Unorm_Pack32") return rhi::Format::eA8B8G8R8_Unorm_Pack32;
        if (format == "eA8B8G8R8_Snorm_Pack32") return rhi::Format::eA8B8G8R8_Snorm_Pack32;
        if (format == "eA8B8G8R8_Uscaled_Pack32") return rhi::Format::eA8B8G8R8_Uscaled_Pack32;
        if (format == "eA8B8G8R8_Sscaled_Pack32") return rhi::Format::eA8B8G8R8_Sscaled_Pack32;
        if (format == "eA8B8G8R8_Uint_Pack32") return rhi::Format::eA8B8G8R8_Uint_Pack32;
        if (format == "eA8B8G8R8_Sint_Pack32") return rhi::Format::eA8B8G8R8_Sint_Pack32;
        if (format == "eA8B8G8R8_Srgb_Pack32") return rhi::Format::eA8B8G8R8_Srgb_Pack32;

        if (format == "eA2R10G10B10_Unorm_Pack32") return rhi::Format::eA2R10G10B10_Unorm_Pack32;
        if (format == "eA2R10G10B10_Snorm_Pack32") return rhi::Format::eA2R10G10B10_Snorm_Pack32;
        if (format == "eA2R10G10B10_Uscaled_Pack32") return rhi::Format::eA2R10G10B10_Uscaled_Pack32;
        if (format == "eA2R10G10B10_Sscaled_Pack32") return rhi::Format::eA2R10G10B10_Sscaled_Pack32;
        if (format == "eA2R10G10B10_Uint_Pack32") return rhi::Format::eA2R10G10B10_Uint_Pack32;
        if (format == "eA2R10G10B10_Sint_Pack32") return rhi::Format::eA2R10G10B10_Sint_Pack32;
        if (format == "eA2B10G10R10_Unorm_Pack32") return rhi::Format::eA2B10G10R10_Unorm_Pack32;
        if (format == "eA2B10G10R10_Snorm_Pack32") return rhi::Format::eA2B10G10R10_Snorm_Pack32;
        if (format == "eA2B10G10R10_Uscaled_Pack32") return rhi::Format::eA2B10G10R10_Uscaled_Pack32;
        if (format == "eA2B10G10R10_Sscaled_Pack32") return rhi::Format::eA2B10G10R10_Sscaled_Pack32;
        if (format == "eA2B10G10R10_Uint_Pack32") return rhi::Format::eA2B10G10R10_Uint_Pack32;
        if (format == "eA2B10G10R10_Sint_Pack32") return rhi::Format::eA2B10G10R10_Sint_Pack32;

        if (format == "eR16_Unorm") return rhi::Format::eR16_Unorm;
        if (format == "eR16_Snorm") return rhi::Format::eR16_Snorm;
        if (format == "eR16_Uscaled") return rhi::Format::eR16_Uscaled;
        if (format == "eR16_Sscaled") return rhi::Format::eR16_Sscaled;
        if (format == "eR16_Uint") return rhi::Format::eR16_Uint;
        if (format == "eR16_Sint") return rhi::Format::eR16_Sint;
        if (format == "eR16_Sfloat") return rhi::Format::eR16_Sfloat;
        if (format == "eR16G16_Unorm") return rhi::Format::eR16G16_Unorm;
        if (format == "eR16G16_Snorm") return rhi::Format::eR16G16_Snorm;
        if (format == "eR16G16_Uscaled") return rhi::Format::eR16G16_Uscaled;
        if (format == "eR16G16_Sscaled") return rhi::Format::eR16G16_Sscaled;
        if (format == "eR16G16_Uint") return rhi::Format::eR16G16_Uint;
        if (format == "eR16G16_Sint") return rhi::Format::eR16G16_Sint;
        if (format == "eR16G16_Sfloat") return rhi::Format::eR16G16_Sfloat;
        if (format == "eR16G16B16_Unorm") return rhi::Format::eR16G16B16_Unorm;
        if (format == "eR16G16B16_Snorm") return rhi::Format::eR16G16B16_Snorm;
        if (format == "eR16G16B16_Uscaled") return rhi::Format::eR16G16B16_Uscaled;
        if (format == "eR16G16B16_Sscaled") return rhi::Format::eR16G16B16_Sscaled;
        if (format == "eR16G16B16_Uint") return rhi::Format::eR16G16B16_Uint;
        if (format == "eR16G16B16_Sint") return rhi::Format::eR16G16B16_Sint;
        if (format == "eR16G16B16_Sfloat") return rhi::Format::eR16G16B16_Sfloat;
        if (format == "eR16G16B16A16_Unorm") return rhi::Format::eR16G16B16A16_Unorm;
        if (format == "eR16G16B16A16_Snorm") return rhi::Format::eR16G16B16A16_Snorm;
        if (format == "eR16G16B16A16_Uscaled") return rhi::Format::eR16G16B16A16_Uscaled;
        if (format == "eR16G16B16A16_Sscaled") return rhi::Format::eR16G16B16A16_Sscaled;
        if (format == "eR16G16B16A16_Uint") return rhi::Format::eR16G16B16A16_Uint;
        if (format == "eR16G16B16A16_Sint") return rhi::Format::eR16G16B16A16_Sint;
        if (format == "eR16G16B16A16_Sfloat") return rhi::Format::eR16G16B16A16_Sfloat;

        if (format == "eR32_Uint") return rhi::Format::eR32_Uint;
        if (format == "eR32_Sint") return rhi::Format::eR32_Sint;
        if (format == "eR32_Sfloat") return rhi::Format::eR32_Sfloat;
        if (format == "eR32G32_Uint") return rhi::Format::eR32G32_Uint;
        if (format == "eR32G32_Sint") return rhi::Format::eR32G32_Sint;
        if (format == "eR32G32_Sfloat") return rhi::Format::eR32G32_Sfloat;
        if (format == "eR32G32B32_Uint") return rhi::Format::eR32G32B32_Uint;
        if (format == "eR32G32B32_Sint") return rhi::Format::eR32G32B32_Sint;
        if (format == "eR32G32B32_Sfloat") return rhi::Format::eR32G32B32_Sfloat;
        if (format == "eR32G32B32A32_Uint") return rhi::Format::eR32G32B32A32_Uint;
        if (format == "eR32G32B32A32_Sint") return rhi::Format::eR32G32B32A32_Sint;
        if (format == "eR32G32B32A32_Sfloat") return rhi::Format::eR32G32B32A32_Sfloat;

        if (format == "eR64_Uint") return rhi::Format::eR64_Uint;
        if (format == "eR64_Sint") return rhi::Format::eR64_Sint;
        if (format == "eR64_Sfloat") return rhi::Format::eR64_Sfloat;
        if (format == "eR64G64_Uint") return rhi::Format::eR64G64_Uint;
        if (format == "eR64G64_Sint") return rhi::Format::eR64G64_Sint;
        if (format == "eR64G64_Sfloat") return rhi::Format::eR64G64_Sfloat;
        if (format == "eR64G64B64_Uint") return rhi::Format::eR64G64B64_Uint;
        if (format == "eR64G64B64_Sint") return rhi::Format::eR64G64B64_Sint;
        if (format == "eR64G64B64_Sfloat") return rhi::Format::eR64G64B64_Sfloat;
        if (format == "eR64G64B64A64_Uint") return rhi::Format::eR64G64B64A64_Uint;
        if (format == "eR64G64B64A64_Sint") return rhi::Format::eR64G64B64A64_Sint;
        if (format == "eR64G64B64A64_Sfloat") return rhi::Format::eR64G64B64A64_Sfloat;

        if (format == "eB10G11R11_Ufloat_Pack32") return rhi::Format::eB10G11R11_Ufloat_Pack32;
        if (format == "eE5B9G9R9_Ufloat_Pack32") return rhi::Format::eE5B9G9R9_Ufloat_Pack32;
        if (format == "eD16_Unorm") return rhi::Format::eD16_Unorm;
        if (format == "eX8_D24_Unorm_Pack32") return rhi::Format::eX8_D24_Unorm_Pack32;
        if (format == "eD32_Sfloat") return rhi::Format::eD32_Sfloat;
        if (format == "eS8_Uint") return rhi::Format::eS8_Uint;
        if (format == "eD16_Unorm_S8_Uint") return rhi::Format::eD16_Unorm_S8_Uint;
        if (format == "eD24_Unorm_S8_Uint") return rhi::Format::eD24_Unorm_S8_Uint;
        if (format == "eD32_Sfloat_S8_Uint") return rhi::Format::eD32_Sfloat_S8_Uint;

        if (format == "eBC1_RGB_Unorm_Block") return rhi::Format::eBC1_RGB_Unorm_Block;
        if (format == "eBC1_RGB_Srgb_Block") return rhi::Format::eBC1_RGB_Srgb_Block;
        if (format == "eBC1_RGBA_Unorm_Block") return rhi::Format::eBC1_RGBA_Unorm_Block;
        if (format == "eBC1_RGBA_Srgb_Block") return rhi::Format::eBC1_RGBA_Srgb_Block;
        if (format == "eBC2_Unorm_Block") return rhi::Format::eBC2_Unorm_Block;
        if (format == "eBC2_Srgb_Block") return rhi::Format::eBC2_Srgb_Block;
        if (format == "eBC3_Unorm_Block") return rhi::Format::eBC3_Unorm_Block;
        if (format == "eBC3_Srgb_Block") return rhi::Format::eBC3_Srgb_Block;
        if (format == "eBC4_Unorm_Block") return rhi::Format::eBC4_Unorm_Block;
        if (format == "eBC4_Snorm_Block") return rhi::Format::eBC4_Snorm_Block;
        if (format == "eBC5_Unorm_Block") return rhi::Format::eBC5_Unorm_Block;
        if (format == "eBC5_Snorm_Block") return rhi::Format::eBC5_Snorm_Block;
        if (format == "eBC6H_Ufloat_Block") return rhi::Format::eBC6H_Ufloat_Block;
        if (format == "eBC6H_Sfloat_Block") return rhi::Format::eBC6H_Sfloat_Block;
        if (format == "eBC7_Unorm_Block") return rhi::Format::eBC7_Unorm_Block;
        if (format == "eBC7_Srgb_Block") return rhi::Format::eBC7_Srgb_Block;

        if (format == "eETC2_R8G8B8_Unorm_Block") return rhi::Format::eETC2_R8G8B8_Unorm_Block;
        if (format == "eETC2_R8G8B8_Srgb_Block") return rhi::Format::eETC2_R8G8B8_Srgb_Block;
        if (format == "eETC2_R8G8B8A1_Unorm_Block") return rhi::Format::eETC2_R8G8B8A1_Unorm_Block;
        if (format == "eETC2_R8G8B8A1_Srgb_Block") return rhi::Format::eETC2_R8G8B8A1_Srgb_Block;
        if (format == "eETC2_R8G8B8A8_Unorm_Block") return rhi::Format::eETC2_R8G8B8A8_Unorm_Block;
        if (format == "eETC2_R8G8B8A8_Srgb_Block") return rhi::Format::eETC2_R8G8B8A8_Srgb_Block;

        if (format == "eEAC_R11_Unorm_Block") return rhi::Format::eEAC_R11_Unorm_Block;
        if (format == "eEAC_R11_Snorm_Block") return rhi::Format::eEAC_R11_Snorm_Block;
        if (format == "eEAC_R11G11_Unorm_Block") return rhi::Format::eEAC_R11G11_Unorm_Block;
        if (format == "eEAC_R11G11_Snorm_Block") return rhi::Format::eEAC_R11G11_Snorm_Block;

        if (format == "eASTC_4x4_Unorm_Block") return rhi::Format::eASTC_4x4_Unorm_Block;
        if (format == "eASTC_4x4_Srgb_Block") return rhi::Format::eASTC_4x4_Srgb_Block;
        if (format == "eASTC_5x4_Unorm_Block") return rhi::Format::eASTC_5x4_Unorm_Block;
        if (format == "eASTC_5x4_Srgb_Block") return rhi::Format::eASTC_5x4_Srgb_Block;
        if (format == "eASTC_5x5_Unorm_Block") return rhi::Format::eASTC_5x5_Unorm_Block;
        if (format == "eASTC_5x5_Srgb_Block") return rhi::Format::eASTC_5x5_Srgb_Block;
        if (format == "eASTC_6x5_Unorm_Block") return rhi::Format::eASTC_6x5_Unorm_Block;
        if (format == "eASTC_6x5_Srgb_Block") return rhi::Format::eASTC_6x5_Srgb_Block;
        if (format == "eASTC_6x6_Unorm_Block") return rhi::Format::eASTC_6x6_Unorm_Block;
        if (format == "eASTC_6x6_Srgb_Block") return rhi::Format::eASTC_6x6_Srgb_Block;
        if (format == "eASTC_8x5_Unorm_Block") return rhi::Format::eASTC_8x5_Unorm_Block;
        if (format == "eASTC_8x5_Srgb_Block") return rhi::Format::eASTC_8x5_Srgb_Block;
        if (format == "eASTC_8x6_Unorm_Block") return rhi::Format::eASTC_8x6_Unorm_Block;
        if (format == "eASTC_8x6_Srgb_Block") return rhi::Format::eASTC_8x6_Srgb_Block;
        if (format == "eASTC_8x8_Unorm_Block") return rhi::Format::eASTC_8x8_Unorm_Block;
        if (format == "eASTC_8x8_Srgb_Block") return rhi::Format::eASTC_8x8_Srgb_Block;
        if (format == "eASTC_10x5_Unorm_Block") return rhi::Format::eASTC_10x5_Unorm_Block;
        if (format == "eASTC_10x5_Srgb_Block") return rhi::Format::eASTC_10x5_Srgb_Block;
        if (format == "eASTC_10x6_Unorm_Block") return rhi::Format::eASTC_10x6_Unorm_Block;
        if (format == "eASTC_10x6_Srgb_Block") return rhi::Format::eASTC_10x6_Srgb_Block;
        if (format == "eASTC_10x8_Unorm_Block") return rhi::Format::eASTC_10x8_Unorm_Block;
        if (format == "eASTC_10x8_Srgb_Block") return rhi::Format::eASTC_10x8_Srgb_Block;
        if (format == "eASTC_10x10_Unorm_Block") return rhi::Format::eASTC_10x10_Unorm_Block;
        if (format == "eASTC_10x10_Srgb_Block") return rhi::Format::eASTC_10x10_Srgb_Block;
        if (format == "eASTC_12x10_Unorm_Block") return rhi::Format::eASTC_12x10_Unorm_Block;
        if (format == "eASTC_12x10_Srgb_Block") return rhi::Format::eASTC_12x10_Srgb_Block;
        if (format == "eASTC_12x12_Unorm_Block") return rhi::Format::eASTC_12x12_Unorm_Block;
        if (format == "eASTC_12x12_Srgb_Block") return rhi::Format::eASTC_12x12_Srgb_Block;

        return rhi::Format::eUndefined;
    }

    rhi::VertexInputRate parseVertexInputRate(std::string_view rate) noexcept {
         if (rate == "eVertex") return rhi::VertexInputRate::eVertex;
         if (rate == "eInstance") return rhi::VertexInputRate::eInstance;
         return rhi::VertexInputRate::eVertex;
    }

    rhi::DynamicState parseDynamicState(std::string_view state) noexcept {
        rhi::DynamicState result = rhi::DynamicState::eNone;
        
        if (state.contains("eViewport")) {
            result = result | rhi::DynamicState::eViewport;
        }
        if (state.contains("eScissor")) {
            result = result | rhi::DynamicState::eScissor;
        }
        if (state.contains("eLine_Width")) {
            result = result | rhi::DynamicState::eLine_Width;
        }
        if (state.contains("eDepth_Bias")) {
            result = result | rhi::DynamicState::eDepth_Bias;
        }
        if (state.contains("eBlend_Constants")) {
            result = result | rhi::DynamicState::eBlend_Constants;
        }
        if (state.contains("eDepth_Bounds")) {
            result = result | rhi::DynamicState::eDepth_Bounds;
        }
        if (state.contains("eStencil_Compare_Mask")) {
            result = result | rhi::DynamicState::eStencil_Compare_Mask;
        }
        if (state.contains("eStencil_Write_Mask")) {
            result = result | rhi::DynamicState::eStencil_Write_Mask;
        }
        if (state.contains("eStencil_Refference")) {
            result = result | rhi::DynamicState::eStencil_Refference;
        }
        if (state.contains("eCull_Mode")) {
            result = result | rhi::DynamicState::eCull_Mode;
        }
        if (state.contains("eFront_Face")) {
            result = result | rhi::DynamicState::eFront_Face;
        }
        if (state.contains("ePrimitive_Topology")) {
            result = result | rhi::DynamicState::ePrimitive_Topology;
        }
        if (state.contains("eViewport_With_Count")) {
            result = result | rhi::DynamicState::eViewport_With_Count;
        }
        if (state.contains("eScissor_With_Count")) {
            result = result | rhi::DynamicState::eScissor_With_Count;
        }
        if (state.contains("eVertex_Input_Binding_Stride")) {
            result = result | rhi::DynamicState::eVertex_Input_Binding_Stride;
        }
        if (state.contains("eDepth_Test_Enable")) {
            result = result | rhi::DynamicState::eDepth_Test_Enable;
        }
        if (state.contains("eDepth_Write_Enable")) {
            result = result | rhi::DynamicState::eDepth_Write_Enable;
        }
        if (state.contains("eDepth_Compare_Op")) {
            result = result | rhi::DynamicState::eDepth_Compare_Op;
        }
        if (state.contains("eDepth_Bounds_Test_Enable")) {
            result = result | rhi::DynamicState::eDepth_Bounds_Test_Enable;
        }
        if (state.contains("eStencil_Test_Enable")) {
            result = result | rhi::DynamicState::eStencil_Test_Enable;
        }
        if (state.contains("eStencil_Op")) {
            result = result | rhi::DynamicState::eStencil_Op;
        }
        if (state.contains("eRasterizer_Discard_Enable")) {
            result = result | rhi::DynamicState::eRasterizer_Discard_Enable;
        }
        if (state.contains("eDepth_Bias_Enable")) {
            result = result | rhi::DynamicState::eDepth_Bias_Enable;
        }
        if (state.contains("ePrimitive_Restart_Enable")) {
            result = result | rhi::DynamicState::ePrimitive_Restart_Enable;
        }
        if (state.contains("eAll")) {
            result = result | rhi::DynamicState::eAll;
        }

        return result;
    }

    rhi::PolygonMode parsePolygonMode(std::string_view mode) noexcept {
        if (mode == "eFill") { return rhi::PolygonMode::eFill; }
        if (mode == "eLine") { return rhi::PolygonMode::eLine; }
        if (mode == "ePoint") { return rhi::PolygonMode::ePoint; }
        return rhi::PolygonMode::eFill;
    }

    rhi::CullMode parseCullMode(std::string_view mode) noexcept {
        if (mode == "eNone") { return rhi::CullMode::eNone; }
        if (mode == "eBack") { return rhi::CullMode::eBack; }
        if (mode == "eFront") { return rhi::CullMode::eFront; }
        if (mode == "eFront_And_Back") { return rhi::CullMode::eFront_And_Back; }
        return rhi::CullMode::eNone;
    }

    rhi::FrontFace parseFrontFace(std::string_view face) noexcept {
        if (face == "eCounter_Clockwise") { return rhi::FrontFace::eCounter_Clockwise; }
        if (face == "eClockwise") { return rhi::FrontFace::eClockwise; }
        return rhi::FrontFace::eCounter_Clockwise;
    }

    rhi::PrimitiveTopology parsePrimitiveTopology(std::string_view topology) noexcept {
        if (topology == "eLine_Strip") { return rhi::PrimitiveTopology::eLine_Strip; }
        if (topology == "eLine_Strip_With_Adjacency") { return rhi::PrimitiveTopology::eLine_Strip_With_Adjacency; }
        if (topology == "eLines") { return rhi::PrimitiveTopology::eLines; }
        if (topology == "eLines_With_Adjacency") { return rhi::PrimitiveTopology::eLines_With_Adjacency; }
        if (topology == "ePatchs") { return rhi::PrimitiveTopology::ePatchs; }
        if (topology == "ePoints") { return rhi::PrimitiveTopology::ePoints; }
        if (topology == "eTriangle_Fan") { return rhi::PrimitiveTopology::eTriangle_Fan; }
        if (topology == "eTriangle_Strip") { return rhi::PrimitiveTopology::eTriangle_Strip; }
        if (topology == "eTriangle_Strip_With_Adjacency") { return rhi::PrimitiveTopology::eTriangle_Strip_With_Adjacency; }
        if (topology == "eTriangles") { return rhi::PrimitiveTopology::eTriangles; }
        if (topology == "eTriangles_With_Adjacency") { return rhi::PrimitiveTopology::eTriangles_With_Adjacency; }
        assert(false);
        return rhi::PrimitiveTopology::eTriangles;
    }

    rhi::CompareOp parseCompareOp(std::string_view op) noexcept {
        if (op == "eAlways") { return rhi::CompareOp::eAlways; }
        if (op == "eEqual") { return rhi::CompareOp::eEqual; }
        if (op == "eGequal") { return rhi::CompareOp::eGequal; }
        if (op == "eGreater") { return rhi::CompareOp::eGreater; }
        if (op == "eLequal") { return rhi::CompareOp::eLequal; }
        if (op == "eLess") { return rhi::CompareOp::eLess; }
        if (op == "eNever") { return rhi::CompareOp::eNever; }
        if (op == "eNot_Equal") { return rhi::CompareOp::eNot_Equal; }
        assert(false);
        return rhi::CompareOp::eAlways;
    }

    rhi::StencilOp parseStencilOp(std::string_view op) noexcept {
        if (op == "eKeep") { return rhi::StencilOp::eKeep; }
        if (op == "eReplace") { return rhi::StencilOp::eReplace; }
        if (op == "eZero") { return rhi::StencilOp::eZero; }
        if (op == "eInvert") { return rhi::StencilOp::eInvert; }
        if (op == "eDec_And_Clamp") { return rhi::StencilOp::eDec_And_Clamp; }
        if (op == "eDec_And_Wrap") { return rhi::StencilOp::eDec_And_Wrap; }
        if (op == "eInc_And_Clamp") { return rhi::StencilOp::eInc_And_Clamp; }
        if (op == "eInc_And_Wrap") { return rhi::StencilOp::eInc_And_Wrap; }
        assert(false);
        return rhi::StencilOp::eKeep;
    }
    
    rhi::StencilOpState parseStencilOpState(const engine::json::Json & json) noexcept {
        return {
            .compareMask = json["compareMask"].as<uint32_t>(),
            .writeMask = json["writeMask"].as<uint32_t>(),
            .reference = json["reference"].as<uint32_t>(),
            .failOp = parseStencilOp(json["failOp"].as<std::string_view>()),
            .passOp = parseStencilOp(json["passOp"].as<std::string_view>()),
            .depthFailOp = parseStencilOp(json["depthFailOp"].as<std::string_view>()),
            .compareOp = parseCompareOp(json["compareOp"].as<std::string_view>())
        };
    }

    rhi::DepthState parseDepthState(const engine::json::Json & json) noexcept {
        return {
            .depthTestEnable = json["depthTestEnable"].get_boolean(),
            .depthWriteEnable = json["depthWriteEnable"].get_boolean(),
            .depthCompareOp = json.contains("depthCompareOp") ? parseCompareOp(json["depthCompareOp"].as<std::string_view>()) : rhi::CompareOp::eAlways,
            .depthBoundsTestEnable = json.contains("depthBoundsTestEnable") ? json["depthBoundsTestEnable"].get_boolean() : false,
            .minDepthBounds = json.contains("minDepthBounds") ? json["minDepthBounds"].as<float>() : 0.0f,
            .maxDepthBounds = json.contains("maxDepthBounds") ? json["maxDepthBounds"].as<float>() : 1.0f,
        };
    }

    rhi::StencilState parseStencilState(const engine::json::Json & json) noexcept {
        if (json.contains("frontAndBack")) {
            auto stencilOpState = parseStencilOpState(json["frontAndBack"].as<std::string_view>());
            return {
                .front = stencilOpState,
                .back = stencilOpState,
                .stencilTestEnable = json["stencilTestEnable"].get_boolean()
            };
        } else {
            return {
                .front = parseStencilOpState(json["front"].as<std::string_view>()),
                .back = parseStencilOpState(json["back"].as<std::string_view>()),
                .stencilTestEnable = json["stencilTestEnable"].get_boolean()
            };
        }
    }

    rhi::EncodedBlendMode parseBlendMode(const engine::json::Json & json) noexcept {
        if (json.is_number()) {
            return json.as<uint32_t>();
        } else {
            auto str = json.as<std::string_view>();
            if (str == "blend_none") { return rhi::common_blend_modes::blend_none; }
            if (str == "blend_alpha") { return rhi::common_blend_modes::blend_alpha; }
            if (str == "blend_add") { return rhi::common_blend_modes::blend_add; }
            if (str == "blend_multiply") { return rhi::common_blend_modes::blend_multiply; }
            if (str == "blend_alpha_pma") { return rhi::common_blend_modes::blend_alpha_pma; }
            if (str == "blend_add_pma") { return rhi::common_blend_modes::blend_add_pma; }
            return rhi::common_blend_modes::blend_none;
        }
    }

    rhi::VertexInputAttribute parseVertexInputAttribute(const engine::json::Json & json) noexcept {
        return {
            .location = json["location"].as<uint8_t>(),
            .binding = json["binding"].as<uint8_t>(),
            .format = parseFormat(json["format"].as<std::string_view>()),
            .offset = json["offset"].as<uint32_t>(),
        };
    }

    rhi::VertexInputBinding parseVertexInputBinding(const engine::json::Json & json) noexcept {
        return {
            .stride = json["stride"].as<uint16_t>(),
            .binding = json["binding"].as<uint8_t>(),
            .inputRate = json.contains("inputRate") ? parseVertexInputRate(json["inputRate"].as<std::string_view>()) : rhi::VertexInputRate::eVertex
        };
    }

    rhi::GraphicsPipelineStatesInfo::ViewportInfo parseViewportState(const engine::json::Json & json) noexcept {
        return {
            json["viewportCount"].as<uint32_t>(),
            json["scissorCount"].as<uint32_t>()
        };
    }

    rhi::GraphicsPipelineStatesInfo::RasterisationInfo parseRasterisationInfo(const engine::json::Json & json) noexcept {
        return {
                json.contains("polygonMode") ? parsePolygonMode(json["polygonMode"].as<std::string_view>()) : rhi::PolygonMode::eFill,
                json.contains("cullmode") ? parseCullMode(json["cullmode"].as<std::string_view>()) : rhi::CullMode::eNone,
                json.contains("frontFace") ? parseFrontFace(json["frontFace"].as<std::string_view>()) : rhi::FrontFace::eCounter_Clockwise,

                json.contains("depthBiasEnable") ? json["depthBiasEnable"].get_boolean() : false,
                json.contains("depthBiasFactor") ? json["depthBiasFactor"].as<float>() : 0.0f,
                json.contains("depthBiasSlopeFactor") ? json["depthBiasSlopeFactor"].as<float>() : 0.0f,
                json.contains("depthBiasClamp") ? json["depthBiasClamp"].as<float>() : 0.0f
            };
    }

    rhi::GraphicsPipelineStatesInfo::MultisampleStateInfo parseMultisampleStateInfo(const engine::json::Json & /*json*/) noexcept {
        // todo!
        return {};
    }

    rhi::GraphicsPipelineStatesInfo::InputAssemblyStateInfo parseInputAssemblyStateInfo(const engine::json::Json & json) noexcept {
        return {
            .topology = parsePrimitiveTopology(json["topology"].as<std::string_view>()),
            .restartEnabled = json.contains("restartEnabled") ? json["restartEnabled"].get_boolean() : false
        };
    }

    rhi::GraphicsPipelineStatesInfo::DepthStencilStateInfo parseDepthStencilStateInfo(const engine::json::Json & json) noexcept {
        rhi::GraphicsPipelineStatesInfo::DepthStencilStateInfo result{};
        if (json.contains("depth")) {
            result.depth = parseDepthState(json["depth"]);
        }

        if (json.contains("stencil")) {
            result.stencil = parseStencilState(json["depth"]);
        }
        return result;
    }

    rhi::GraphicsPipelineStatesInfo::TessellationStateInfo parseTessellationStateInfo(const engine::json::Json & json) noexcept {
        return {
            .patchControlPoints = json["patchControlPoints"].as<uint32_t>()
        };
    }

    stl::vector<rhi::EncodedBlendMode> parseBlendModes(const engine::json::Array & array) noexcept {
        stl::vector<rhi::EncodedBlendMode> modes;
        modes.reserve(array.size());
        for (const auto & value : array) {
            modes.emplace_back(parseBlendMode(value));
        }
        return modes;
    }

    rhi::GraphicsPipelineStatesInfo::VertexInputStateInfo parsevertexInputStateInfo(const engine::json::Json & json) noexcept {
        rhi::GraphicsPipelineStatesInfo::VertexInputStateInfo result;

        const auto & bindingsArray = json["inputBindings"].get_array();
        result.inputBindings.reserve(bindingsArray.size());
        for (const auto & value : bindingsArray) {
            result.inputBindings.emplace_back(parseVertexInputBinding(value));
        }

        const auto & attributesArray = json["attributes"].get_array();
        result.attributes.reserve(attributesArray.size());
        for (const auto & value : attributesArray) {
            result.attributes.emplace_back(parseVertexInputAttribute(value));
        }

        return result;
    }

    rhi::GraphicsPipelineStatesInfo parseGraphicsPipelineInfo(const engine::json::Json & json) noexcept {
        return {
            .viewportState = json.contains("viewportState") ? parseViewportState(json["viewportState"]) : rhi::GraphicsPipelineStatesInfo::ViewportInfo{},
            .dynamicStates = json.contains("dynamicStates") ?  parseDynamicState(json["dynamicStates"].as<std::string_view>()) :  rhi::DynamicState::eViewport | rhi::DynamicState::eScissor,
            .rasterisationState = json.contains("rasterisationState") ?  parseRasterisationInfo(json["rasterisationState"]) : rhi::GraphicsPipelineStatesInfo::RasterisationInfo{},
            .multisampleState = json.contains("multisampleState") ?  parseMultisampleStateInfo(json["multisampleState"]) : rhi::GraphicsPipelineStatesInfo::MultisampleStateInfo{},
            .inputAssemblyState = json.contains("inputAssemblyState") ?  parseInputAssemblyStateInfo(json["inputAssemblyState"]) : rhi::GraphicsPipelineStatesInfo::InputAssemblyStateInfo{},
            .depthStencilState  = json.contains("depthStencilState") ?  parseDepthStencilStateInfo(json["depthStencilState"]) : rhi::GraphicsPipelineStatesInfo::DepthStencilStateInfo{},
            .tessellationState  = json.contains("tessellationState") ?  parseTessellationStateInfo(json["tessellationState"]) : rhi::GraphicsPipelineStatesInfo::TessellationStateInfo{},
            .blendModes = json.contains("blendModes") ?  parseBlendModes(json["blendModes"].get_array()) : stl::vector<rhi::EncodedBlendMode>{},
            .vertexInputState = json.contains("vertexInputState") ?  parsevertexInputStateInfo(json["vertexInputState"]) : rhi::GraphicsPipelineStatesInfo::VertexInputStateInfo{}
        };
    }
}