#pragma once

#include "tile_id.h"
#include "engine/src/math/math.h"

namespace map {

float getHeightFromEncodedRGB(engine::vec3f rgb) noexcept {
    return -10000.0f + ((rgb.x * 256.0f * 256.0f + rgb.y * 256.0f + rgb.z) * 0.1f);
}

TileID latLonToTileID(double lat, double lon, uint8_t zoom) noexcept {
    // Переводим широту из градусов в радианы
    const double latRad = lat * engine::pi_v<double> / 180.0;
    const double n = std::pow(2.0, zoom);
    
    return {
        .x = static_cast<uint32_t>((lon + 180.0) / 360.0 * n),
        .y = static_cast<uint32_t>((1.0 - std::log(std::tan(latRad) + (1.0 / std::cos(latRad))) / engine::pi_v<double>) / 2.0 * n),
        .z = zoom
    };
}

TileID normalizedMercatorToTileId(engine::vec2d mercator, uint8_t zoom) noexcept {
    const uint32_t tilesPerAxis = 1u << zoom;
    return {
        .x = static_cast<uint32_t>(mercator.x * tilesPerAxis),
        .y = static_cast<uint32_t>(mercator.y * tilesPerAxis),
        .z = zoom
    };
}


}