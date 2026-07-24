#pragma once

#include "tile_id.h"
#include "engine/src/math/math.h"

namespace map {

inline float getHeightFromEncodedRGB(engine::vec3f rgb) noexcept {
    return -10000.0f + ((rgb.x * 256.0f * 256.0f + rgb.y * 256.0f + rgb.z) * 0.1f);
}

inline TileID latLonToTileID(double lat, double lon, uint8_t zoom) noexcept {
    // Переводим широту из градусов в радианы
    const double latRad = lat * engine::pi_v<double> / 180.0;
    const double n = std::pow(2.0, zoom);
    
    return {
        .x = static_cast<uint32_t>((lon + 180.0) / 360.0 * n),
        .y = static_cast<uint32_t>((1.0 - std::log(std::tan(latRad) + (1.0 / std::cos(latRad))) / engine::pi_v<double>) / 2.0 * n),
        .z = zoom
    };
}

inline TileID normalizedMercatorToTileId(engine::vec2d mercator, uint8_t zoom) noexcept {
    const uint32_t tilesPerAxis = 1u << zoom;
    return {
        .x = static_cast<uint32_t>(mercator.x * tilesPerAxis),
        .y = static_cast<uint32_t>(mercator.y * tilesPerAxis),
        .z = zoom
    };
}

inline engine::vec2f tileToGeo(TileID tileId, float inTileX = 0.5f, float inTileY = 0.5f) noexcept {    
    // Количество тайлов на текущем уровне зума
    const auto n = 1 << tileId.z;
    
    const float centerX = tileId.x + inTileX;
    const float centerY = tileId.y + inTileY;
    
    const float lon = centerX / n * 360.0f - 180.0f;
    const float lat_rad = std::atan(std::sinh(engine::pi * (1.0f - 2.0f * centerY / n)));

    return {lat_rad * 180.0f / engine::pi, lon};
}

inline float metersToTileUnits(float meters, float latitude, uint8_t zoom, float extent) noexcept {
    constexpr double kEarthCircumference = 40075016.6863;
    
    const auto latRad = (latitude * engine::pi) / 180.0f;
    
    // tilesize in metters on latitude
    const auto tileSizeInMeters = (kEarthCircumference * std::cos(latRad)) / (1u << zoom);
    const auto unitsPerMeter = extent / tileSizeInMeters;

    return static_cast<float>(meters * unitsPerMeter);
}

}