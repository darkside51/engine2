#pragma once

#include "meshoptimizer.h"
#include <cstdint>

namespace engine::graphics {

uint16_t float_to_uint16(float v) noexcept {
    return meshopt_quantizeHalf(v);
}

float uint16_to_float(uint16_t v) noexcept {
    return meshopt_dequantizeHalf(v);
}

int32_t float_to_snorm(float v, int n) noexcept {
    return meshopt_quantizeSnorm(v, n);
}

int32_t float_to_unorm(float v, int n) noexcept {
    return meshopt_quantizeUnorm(v, n);
}

}