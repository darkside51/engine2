#pragma once

#include "types.h"

#include <cstdint>
#include <vector>

namespace engine::rhi {
    
struct SwapchainCreateInfo {
  bool supportTransferSrc = false;
  bool supportTransferDst = false;
  Format format = Format::eB8G8R8A8_Unorm;
  ColorSpace colorSpace = ColorSpace::eSRGB_Nonlinear;
  uint32_t imagesCount = 1u;
  stl::vector<uint32_t> queueFamilies;
};

}