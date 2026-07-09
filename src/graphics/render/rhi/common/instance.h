#pragma once

#include "common/version.h"
#include "stl/vector.h"

#include <string_view>
#include <vector>

namespace engine::rhi {

struct InstanceCreateInfo {
    common::Version apiVersion;
    stl::vector<std::string_view> extensions;
    stl::vector<std::string_view> layers;
    bool createDebugMessages = false;
    const void * allocationCallback = nullptr;
};

}