#pragma once

#include "graphics/render/rhi/common/renderpass.h"
#include "utils/json.h"

#include <span>

namespace engine {
    rhi::RenderPassDescription parseRenderPass(const engine::json::Json & json, std::span<const engine::rhi::Format> formats);
}