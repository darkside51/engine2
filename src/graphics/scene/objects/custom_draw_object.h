#pragma once

#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/scene/object.h"
#include "graphics/scene/types.h"
#include "math/math.h"

#include <cstdint>
#include <type_traits>

namespace engine::scene {

class Scene;
struct ViewParams;

class ICustomDrawObjectRenderer;

class CustomDrawObject : public AttachmentObject {
public:
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomDrawObjectRenderer, std::decay_t<T>>>>
    CustomDrawObject(T&&, void *) = delete;

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomDrawObjectRenderer, std::decay_t<T>>>>
    CustomDrawObject(const T & renderer, void * data) : 
        CustomDrawObject(
            ctti::typeIndex<ICustomDrawObjectRenderer, T>()/*may be rtti::...?*/, 
            renderer, data
        ) {}

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomDrawObjectRenderer, std::decay_t<T>>>>
    CustomDrawObject(T & renderer, void * data) : 
        CustomDrawObject(
            ctti::typeIndex<ICustomDrawObjectRenderer, T>()/*may be rtti::...?*/, 
            renderer, data
        ) {}

    void draw(const Scene & scene, const ViewParams & viewParams, RenderPass pass, rhi::runtime::CommandListPtr & commandList, uint8_t frame) const;
    bool cmp(const CustomDrawObject &) const noexcept;

    void * getData() const noexcept {
        return _customData;
    }

private:
    CustomDrawObject(size_t type, const ICustomDrawObjectRenderer & renderer, void * data);
    const ICustomDrawObjectRenderer & _renderer;
    void * _customData = nullptr;
    uint32_t _subType;
};

class ICustomDrawObjectRenderer {
public:
    virtual ~ICustomDrawObjectRenderer() = default;
    virtual void draw(
        const CustomDrawObject & object, const Scene & scene, const ViewParams & viewParams, 
        RenderPass pass, const mat4f &, rhi::runtime::CommandListPtr & commandList, uint8_t frame
    ) const = 0;
};

}