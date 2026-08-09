#pragma once

#include <cstdint>
#include <string_view>

namespace map {
    struct ColorResult {
        uint32_t color;
        bool result = true;
    };

    ColorResult colorNameToUint32t(std::string_view name) noexcept;
    ColorResult hexStringToUint32(std::string_view sv) noexcept;
}