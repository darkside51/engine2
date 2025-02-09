#pragma once

#include "common/version.h"

#include <string_view>
#include <vector>

namespace engine::rhi {

struct InstanceCreateInfo {
    common::Version apiVersion;
    std::vector<std::string_view> extensions;
    std::vector<std::string_view> layers;
    bool createDebugMessages = false;
    const void * allocationCallback = nullptr;
};

}