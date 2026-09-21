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

class ICustomDrawObjectRenderer;

class CustomDrawObject : public AttachmentObject {
public:
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomDrawObjectRenderer, std::decay_t<T>>>>
    CustomDrawObject(T&&, void *) = delete;

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomDrawObjectRenderer, std::decay_t<T>>>>
    CustomDrawObject(T & renderer, void * data) : 
        CustomDrawObject(
            renderer
        ) {
            _customData = data;
        }

    void draw(const Scene & scene, const ViewParams & viewParams, RenderPass pass, rhi::runtime::CommandListPtr & commandList, uint8_t frame) const;
    void dispatch(const Scene & scene, rhi::runtime::CommandListPtr & commandList, uint8_t frame) const;
    bool cmp(const CustomDrawObject &) const noexcept;

    void * getData() const noexcept {
        return _customData;
    }

    const ICustomDrawObjectRenderer & getDelegate() const noexcept {
        return _renderer;
    }

    ICustomDrawObjectRenderer & getDelegate() noexcept {
        return _renderer;
    }


private:
    CustomDrawObject(ICustomDrawObjectRenderer & renderer);
    ICustomDrawObjectRenderer & _renderer;
    void * _customData = nullptr;
};

class ICustomDrawObjectRenderer {
public:
    ICustomDrawObjectRenderer(bool dispatch = false) : _dispatch(dispatch) {}
    virtual ~ICustomDrawObjectRenderer() = default;
    virtual void draw(
        const CustomDrawObject & object, const Scene & scene, const ViewParams & viewParams, 
        RenderPass pass, const mat4f &, rhi::runtime::CommandListPtr & commandList, uint8_t frame
    ) const = 0;
    virtual void dispatch(
        const CustomDrawObject & object, const Scene & scene, const mat4f &,
        rhi::runtime::CommandListPtr & commandList, uint8_t frame
    ) const {};

    virtual bool compare(const CustomDrawObject & object0, const CustomDrawObject & object1) const noexcept {
        return false;
    }

    inline bool isDispatchEnabled() const noexcept {
        return _dispatch;
    }

    virtual void dispatch(const Scene & scene, rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end) {
        for (auto it = begin; it != end; ++it) { // dirty!!!
            const auto & object = *static_cast<const CustomDrawObject*>(*it);
            object.dispatch(scene, commandList, frame);
        }
    }

    virtual void draw(AttachmentsIterator begin, AttachmentsIterator end, 
        const Scene & scene, const ViewParams & viewParams, RenderPass pass, 
        rhi::runtime::CommandListPtr & commandList, uint8_t frame
    ) {
        for (auto it = begin; it != end; ++it) { // dirty!!!
            const auto & object = *static_cast<const CustomDrawObject*>(*it);
            object.draw(scene, viewParams, pass, commandList, frame);
        }
    }

private:
    bool _dispatch = false;
};

// revision
class ICustomObjectSceneDelegate {
public:
    ICustomObjectSceneDelegate(bool dispatch = false) : _dispatch(dispatch) {}
    virtual ~ICustomObjectSceneDelegate() = default;

    virtual bool compare(const CustomDrawObject & object0, const CustomDrawObject & object1) const noexcept {
        return false;
    }

    virtual void dispatch(const Scene & scene, rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end) = 0;

    virtual void draw(AttachmentsIterator begin, AttachmentsIterator end, 
        const ViewParams & viewParams, rhi::runtime::CommandListPtr & commandList, uint8_t frame
    ) = 0;

    inline bool isDispatchEnabled() const noexcept {
        return _dispatch;
    }
    
private:
    bool _dispatch = false;
};

}