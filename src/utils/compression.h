#pragma once

#include "log/log.h"
#include "zlib.h"

#include <cstddef>
#include <cstdint>
#include <cstring>  // for memset
#include <span>
#include <vector>

namespace engine::compression {

class ZstreamHolder {
public:
    ZstreamHolder() { std::memset(&_zs, 0, sizeof(_zs)); }

    ~ZstreamHolder() {
        if (_inited) {
            inflateEnd(&_zs);
        }
    }

    int initInflate(int windowBits) {
        if (_inited) {
            return Z_OK;
        }

        auto const result = inflateInit2(&_zs, windowBits);
        if (result == Z_OK) {
            _inited = true;
        }
        return result;
    }

    int inflate(int flush) { return ::inflate(&_zs, flush); }

    z_stream_s * operator->() { return &_zs; }
    z_stream_s * operator*() { return &_zs; }

private:
    z_stream _zs;
    bool _inited = false;
};

bool isDataCompressed(stl::span<const std::byte> source) noexcept {
    return source.size() > 2u
        && (
               // zlib
               (static_cast<uint8_t>(source[0u]) == 0x78 &&
                (static_cast<uint8_t>(source[1u]) == 0x9C || 
                 static_cast<uint8_t>(source[1u]) == 0x01 || 
                 static_cast<uint8_t>(source[1u]) == 0xDA || 
                 static_cast<uint8_t>(source[1u]) == 0x5E)) ||
               // gzip
               (static_cast<uint8_t>(source[0u]) == 0x1F && 
                static_cast<uint8_t>(source[1u]) == 0x8B)
            );
}

stl::vector<std::byte> decompress(stl::span<const std::byte> source) {
    ZstreamHolder zs;
    // The windowBits parameter is the base two logarithm of the window size (the size of the
    // history buffer). It should be in the range 8..15 for this version of the library. Larger
    // values of this parameter result in better compression at the expense of memory usage. This
    // range of values also changes the decoding type:
    //  -8 to -15 for raw deflate
    //  8 to 15 for zlib
    // (8 to 15) + 16 for gzip
    // (8 to 15) + 32 to automatically detect gzip/zlib header
    constexpr int windowBits = 15 + 32;  // auto with windowbits of 15

    auto result = zs.initInflate(windowBits);
    if (result == Z_OK) {
        stl::vector<std::byte> destination;
        zs->next_in = reinterpret_cast<Bytef *>(const_cast<std::byte *>(source.data()));
        zs->avail_in = static_cast<uInt>(source.size());

        // get the decompressed bytes blockwise using repeated calls to inflate
        while (result == Z_OK) {
            auto const partSize = 2u * source.size();
            destination.resize(zs->total_out + partSize);

            zs->avail_out = static_cast<uInt>(partSize);
            zs->next_out = reinterpret_cast<Bytef *>(&destination[0u] + zs->total_out);

            result = zs.inflate(Z_NO_FLUSH);

            if (result != Z_STREAM_END && result != Z_OK && result != Z_BUF_ERROR) {
                LOG_E("decompression error: %s", zs->msg);
                return {};
            }
        }
        destination.resize(zs->total_out);
        return destination;
    }
    return {};
}

}