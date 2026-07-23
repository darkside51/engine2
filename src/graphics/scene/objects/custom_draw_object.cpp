#include "custom_draw_object.h"

#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

namespace engine::scene {

CustomDrawObject::CustomDrawObject(Renderer && drawImpl) : 
    AttachmentObject(ctti::typeIndex<AttachmentObject, CustomDrawObject>()), 
    _drawImpl(std::move(drawImpl)) {

}

void CustomDrawObject::draw(const Scene & scene, const ViewParams & viewParams, rhi::runtime::CommandListPtr & commandList, uint8_t frame) const {
    _drawImpl(scene, viewParams, node->getWorldTransform(), commandList, frame);
}

bool CustomDrawObject::cmp(const CustomDrawObject &) const noexcept {
    return false;
}

}