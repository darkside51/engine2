#include "graphics_service.h"

#include "render_pass_manager.h"
#include "pipelines_manager.h"
#include "material.h"
#include "graphics/mesh/model_manager.h"
#include "graphics/texture/texture_manager.h"

namespace engine::graphics {

void GraphicsService::destroy() noexcept {
    _modelManager.destroy();
    _materialManager.destroy();
    _textureManager.destroy();
    _pipelinesManager.destroy();
    _renderPassManager.destroy();
}

}