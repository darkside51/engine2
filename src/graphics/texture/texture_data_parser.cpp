#include "texture_data_parser.h"

#include <sail-c++/sail-c++.h>

namespace engine::graphics {

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

    TextureCreateInfo parseTextureData(std::span<const std::byte> bytes) {
        sail::image_input imageInput(bytes.data(), bytes.size());
        sail::image image = imageInput.next_frame();

        if (!image.is_valid()) {
            return {};
        }
        
        std::span data{static_cast<std::byte*>(image.pixels()), image.pixels_size()}; 

        TextureCreateInfo result {
            .metaInfo = {
                .size = {image.width(), image.height()},
                .bytesPerLine = static_cast<uint16_t>(image.bytes_per_line()),
                .format = convert(image.pixel_format()),
                .bpp = static_cast<uint8_t>(image.bits_per_pixel()),
                .hasPalette = image.palette().is_valid()
            },
            .data = {data.begin(), data.end()}
        };

        return result;
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