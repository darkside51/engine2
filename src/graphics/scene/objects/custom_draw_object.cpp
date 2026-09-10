#include "custom_draw_object.h"

#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

namespace engine::scene {

CustomDrawObject::CustomDrawObject(size_t type, const ICustomDrawObjectRenderer & renderer) : 
    AttachmentObject(ctti::typeIndex<AttachmentObject, CustomDrawObject>()), 
    _renderer(renderer),
    _subType(type) { }

void CustomDrawObject::draw(const Scene & scene, const ViewParams & viewParams, RenderPass pass, rhi::runtime::CommandListPtr & commandList, uint8_t frame) const {
    _renderer.draw(*this, scene, viewParams, pass, node->getWorldTransform(), commandList, frame);
}

bool CustomDrawObject::cmp(const CustomDrawObject & other) const noexcept {
    if (&_renderer != &other._renderer) {
        return std::less<const ICustomDrawObjectRenderer *>{}(&_renderer, &other._renderer);
    }

    return _subType < other._subType;
}

}