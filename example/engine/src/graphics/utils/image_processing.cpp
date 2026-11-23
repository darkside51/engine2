#include "image_processing.h"

#include "bc7enc.h"
#define RGBCX_IMPLEMENTATION
#include "rgbcx.h"

#include <algorithm>
#include <cassert>

namespace engine::graphics {

    class BCXCompression {
    public:
        static BCXCompression & instance() noexcept {
            static BCXCompression instance;
            return instance;
        }

    private:
        BCXCompression() {
            // initialization of global tables (called once before compression)
            bc7enc_compress_block_init();
            // initialization global tables rgbcx
            rgbcx::init();
        }
    };

    // this function extracts a 4x4 pixel block from a flat RGBA array.
    // it automatically fills pixels outside the texture boundaries using the clamp method (edge ​​repetition).
    void extractBlock4x4(const std::byte * srcData, uint8_t bytesPepPixel, uint32_t width, uint32_t height, 
        uint32_t blockX, uint32_t blockY, ColorRGBA dstBlock[16u]) {
        for (uint32_t y = 0u; y < 4u; ++y) {
            const uint32_t pixelY = std::min(blockY * 4u + y, height - 1u);
            for (uint32_t x = 0u; x < 4u; ++x) {
                const uint32_t pixelX = std::min(blockX * 4u + x, width - 1u);
            
                uint32_t srcIdx = (pixelY * width + pixelX) * bytesPepPixel;
                
                dstBlock[y * 4u + x] = {
                    srcData[srcIdx + 0u],
                    srcData[srcIdx + 1u],
                    srcData[srcIdx + 2u],
                    bytesPepPixel == 4u ? srcData[srcIdx + 3u] : std::byte{0xffu}
                };
            }
        }
    }

    stl::vector<std::byte> CompressToBC7(
        const std::byte * srcData, uint8_t bytesPepPixel, 
        uint32_t width, uint32_t height, 
        int32_t maxPartitionsToScan = BC7ENC_MAX_PARTITIONS1, int32_t uberLevel = BC7ENC_MAX_UBER_LEVEL
    ) {
        bc7enc_compress_block_params params;
        bc7enc_compress_block_params_init(&params);

        params.m_max_partitions_mode = std::min(BC7ENC_MAX_PARTITIONS1, maxPartitionsToScan);
        params.m_uber_level = std::min(BC7ENC_MAX_UBER_LEVEL, uberLevel);

        if (bytesPepPixel == 3u) {
            // inform the encoder that the alpha channel is 255 everywhere.
            // this allows bc7enc to avoid wasting bits and time encoding transparency.
            bc7enc_compress_block_params_init_linear_weights(&params);
        }

        const uint32_t blocksX = (width + 3u) / 4u;
        const uint32_t blocksY = (height + 3u) / 4u;
        
        // BC7 use 16 bytes on 4х4 block
        stl::vector<std::byte> outData(blocksX * blocksY * 16u);

        ColorRGBA rgbaBlock[16u];

        for (uint32_t by = 0u; by < blocksY; ++by) {
            for (uint32_t bx = 0u; bx < blocksX; ++bx) {
                extractBlock4x4(srcData, bytesPepPixel, width, height, bx, by, rgbaBlock);
                auto * dstPtr = &outData[(by * blocksX + bx) * 16u];
                bc7enc_compress_block(dstPtr, &rgbaBlock[0u], &params);
            }
        }

        return outData;
    }

    stl::vector<std::byte> CompressToBC1Or3(
        const std::byte * srcData, uint8_t bytesPepPixel,
        uint32_t width, uint32_t height, 
        bool useBc3, uint32_t qualityLevel = 10u
    ) {
        // quality level for color tuple selection (0 to 18, where 18 is maximum / excessive)
        // the optimal speed / quality balance is level 10 - 12.

        const uint32_t blocksX = (width + 3u) / 4u;
        const uint32_t blocksY = (height + 3u) / 4u;
        
        const uint32_t bytesPerBlock = useBc3 ? 16u : 8u;
        stl::vector<std::byte> outData(blocksX * blocksY * bytesPerBlock);

        ColorRGBA rgbaBlock[16u];
        
        for (uint32_t by = 0u; by < blocksY; ++by) {
            for (uint32_t bx = 0u; bx < blocksX; ++bx) {
                extractBlock4x4(srcData, bytesPepPixel, width, height, bx, by, rgbaBlock);
                auto * dstPtr = &outData[(by * blocksX + bx) * bytesPerBlock];

                if (useBc3) {
                    // BC3: compress color and alpha separately (16 bytes total)
                    // rgbcx accepts the pixel stride (in this case, 4 bytes for the ColorRGBA structure)
                    rgbcx::encode_bc3(qualityLevel, dstPtr, reinterpret_cast<const uint8_t*>(rgbaBlock));
                } else {
                    // BC1: compresses color only (8 bytes); the alpha channel is discarded.
                    rgbcx::encode_bc1(qualityLevel, dstPtr, reinterpret_cast<const uint8_t*>(rgbaBlock), false, false);
                }
            }
        }

        return outData;
    }

    stl::vector<std::byte> CompressToBC4Or5(
        const std::byte * srcData, uint8_t bytesPepPixel, 
        uint32_t width, uint32_t height, bool useBc5) {
        const uint32_t blocksX = (width + 3u) / 4u;
        const uint32_t blocksY = (height + 3u) / 4u;
        
        const uint32_t bytesPerBlock = useBc5 ? 16u : 8u;
        stl::vector<std::byte> outData(blocksX * blocksY * bytesPerBlock);

        ColorRGBA rgbaBlock[16u];

        for (uint32_t by = 0u; by < blocksY; ++by) {
            for (uint32_t bx = 0u; bx < blocksX; ++bx) {
                extractBlock4x4(srcData, bytesPepPixel, width, height, bx, by, rgbaBlock);
                auto * dstPtr = &outData[(by * blocksX + bx) * bytesPerBlock];

                if (useBc5) {
                    // BC5: compress R and G channels independently (useful for Normal Maps)
                    // parameters: destination, pointer to pixel block, channel indices (0=R, 1=G), stride (4 bytes)
                    rgbcx::encode_bc5(dstPtr, reinterpret_cast<const uint8_t*>(rgbaBlock), 0u, 1u, 4u);
                } else {
                    // BC4: compress only one channel (e.g., red for roughness)
                    rgbcx::encode_bc4(dstPtr, reinterpret_cast<const uint8_t*>(rgbaBlock), 4u);
                }
            }
        }

        return outData;
    }

    stl::vector<std::byte> compressToBC(
        const std::byte * srcData, uint8_t bytesPepPixel, 
        uint32_t width, uint32_t height, BC_Compression compression
    ) {
        BCXCompression::instance();

        switch (compression) {
            case BC_Compression::eBC1:
                return CompressToBC1Or3(srcData, bytesPepPixel, width, height, false);
            case BC_Compression::eBC3:
                return CompressToBC1Or3(srcData, bytesPepPixel, width, height, true);
            case BC_Compression::eBC4:
                return CompressToBC4Or5(srcData, bytesPepPixel, width, height, false);
            case BC_Compression::eBC5:
                return CompressToBC4Or5(srcData, bytesPepPixel, width, height, true);
            case BC_Compression::eBC6:
                assert(false); // unsupported now!
                return {};
            case BC_Compression::eBC7:
                return CompressToBC7(srcData, bytesPepPixel, width, height);
        }

        assert(false);
        return {};
    }

}