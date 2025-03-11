#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace engine::rhi {
    // template <typename T>
    // inline T operator | (T a, T b) {
    //     return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
    // }

    // template <typename T>
    // inline T operator & (T a, T b) {
    //     return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
    // }

    struct Offset2D {
        int32_t x;
        int32_t y;
    };
    
    struct Offset3D {
        int32_t x;
        int32_t y;
        int32_t z;
    };
    
    struct Extent2D {
        uint32_t width = 0u;
        uint32_t height = 0u;
    };

    struct Extent3D {
        uint32_t width = 0u;
        uint32_t height = 0u;
        uint32_t depth = 0u;
    };
    
    struct ClearColorValue {
        float value[4];
    };
    
    struct ClearDepthStencilValue {
        float depth;
        uint32_t stencil;
    };
    
    union ClearValue {
        ClearColorValue color;
        ClearDepthStencilValue depthStencil;

        explicit ClearValue(uint32_t color) : 
        color{
            static_cast<float>(0xffu & (color >> 24u)) / 255.0f, 
            static_cast<float>(0xffu & (color >> 16u)) / 255.0f, 
            static_cast<float>(0xffu & (color >> 8u)) / 255.0f, 
            static_cast<float>(0xffu & (color >> 0u)) / 255.0f
        } {}

        ClearValue(float r, float g, float b, float a) :  color{r, g, b, a} {}   
        ClearValue(float depth, uint32_t stencil) : depthStencil{depth, stencil} {}
    };

    enum class MemoryPropertyFlags :uint8_t {
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
        eDepthStencil = eDepth | eColor
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
        Extent3D extent;
        uint8_t mipLevels = 1u;
        uint8_t arrayLayers = 1u;
        uint8_t samples = 1u;
        std::vector<uint32_t> queueFamilyIndices = {};
    };

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
        
        AttachmentLoadStoreOp() : mask(std::to_underlying(AttachmentLoadOp::eDont_Care) | 
                                      (std::to_underlying(AttachmentStoreOp::eDont_Care) << 4u)) {}

        AttachmentLoadStoreOp(AttachmentLoadOp loadOp, AttachmentStoreOp storeOp) : mask(std::to_underlying(loadOp) | 
                                                                                        (std::to_underlying(storeOp) << 4u)) {}

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
        std::pair<const void*, size_t> pNext = {nullptr, 0u}; // value + hash(value) 
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
        eINPUT_Attachment_Read = 0x00000010u,
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
        eColor_Attachment_Read_NONCOHERENT_Ext = 0x00080000u,
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
        eBy_Region = 0x00000001u,
        eDEevice_Group = 0x00000004u,
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

    enum class FenceCreateFlags : uint8_t {
        eNone = 0u,
        eSignaled = 1u
    };

    enum class SubpassContents : uint8_t {
        ePrimaryBufferInline = 0u,
        eSecondaryBuffers = 1u
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

    enum class BlendOp : uint8_t {
        eAdd = 0u,
        eSubtract = 1u,
        eReverse_Subtract = 2u,
        eMin = 3u,
        eMax = 4u,
    };

    enum class BlendFactor : uint8_t {
        eZero = 0u,
        eOne = 1u,
        eSrc_Color = 2u,
        eOne_Minus_Src_Color = 3u,
        eDst_Color = 4u,
        eOne_Minus_Dst_Color = 5u,
        eSrc_Alpha = 6u,
        eOne_Minus_Src_Alpha = 7u,
        eDst_Alpha= 8u,
        eOne_Minus_Dst_Alpha = 9u,
        eConst_Color = 10u,
        eOne_Minus_Const_Color = 11u,
        eConst_Alpha = 12u,
        eOne_Minus_Const_Alpha = 13u,
        eSrc_Alpha_Saturate = 14u,
        eSrc1_Color = 15u,
        eOne_Minus_Src1_Color = 16u,
        eSrc1_Alpha = 17u,
        eOne_Minus_Src1_Alpha = 18u,
    };
}