#pragma once

#include "graphics/render/rhi/common/pipeline.h"
#include "utils/json.h"

#include <span>

namespace engine {
    rhi::GraphicsPipelineStatesInfo parseGraphicsPipelineInfo(const engine::json::Json & json) noexcept;
}