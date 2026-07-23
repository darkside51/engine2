#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/scene/object.h"
#include "math/math.h"

#include <cstdint>
#include <functional>

namespace engine::scene {

class Scene;
struct ViewParams;

class CustomDrawObject : public AttachmentObject {
    using Renderer = std::move_only_function<void(const Scene & scene, const ViewParams &, const mat4f &, rhi::runtime::CommandListPtr &, uint8_t) const>;

public:
    CustomDrawObject(Renderer && drawImpl);

    void draw(const Scene & scene, const ViewParams & viewParams, rhi::runtime::CommandListPtr & commandList, uint8_t frame) const;

    bool cmp(const CustomDrawObject &) const noexcept;

private:
    Renderer _drawImpl;
};

}