#pragma once

#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/scene/object.h"
#include "graphics/scene/types.h"
#include "math/math.h"

#include <chrono>
#include <cstdint>
#include <type_traits>

namespace engine::scene {

class Scene;
class ICustomSceneDelegate;

class CustomSceneObject : public AttachmentObject {
public:
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomSceneDelegate, std::decay_t<T>>>>
    CustomSceneObject(T &&, T::object_type *) = delete;

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<ICustomSceneDelegate, std::decay_t<T>>>>
    CustomSceneObject(T & delegate, T::object_type * data) : CustomSceneObject(delegate, static_cast<void*>(data)) {}

    const ICustomSceneDelegate & getDelegate() const noexcept {
        return _delegate;
    }

    ICustomSceneDelegate & getDelegate() noexcept {
        return _delegate;
    }

    void * getData() const noexcept {
        return _customData;
    }

    bool cmp(const CustomSceneObject &) const noexcept;

private:
    CustomSceneObject(ICustomSceneDelegate & delegate, void * data);
    ICustomSceneDelegate & _delegate;
    void * _customData = nullptr;
};

enum CustomSceneDelegateFlags : uint8_t {
    eNone = 0u,
    eDraw = 1u,
    eDispatch = 2u
};

inline constexpr CustomSceneDelegateFlags operator | (CustomSceneDelegateFlags a, CustomSceneDelegateFlags b) noexcept {
    return static_cast<CustomSceneDelegateFlags>(std::to_underlying(a) | std::to_underlying(b));
}
inline constexpr CustomSceneDelegateFlags operator & (CustomSceneDelegateFlags a, CustomSceneDelegateFlags b) noexcept {
    return static_cast<CustomSceneDelegateFlags>(std::to_underlying(a) & std::to_underlying(b));
}

class ICustomSceneDelegate {
public:
    ICustomSceneDelegate(CustomSceneDelegateFlags flags = CustomSceneDelegateFlags::eDraw) : _flags(flags) {}
    virtual ~ICustomSceneDelegate() = default;
    
    virtual void dispatch(const Scene & scene, rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end) {};

    virtual void draw(AttachmentsIterator begin, AttachmentsIterator end, 
        const Scene & scene, const ViewParams & viewParams, RenderPass pass, 
        rhi::runtime::CommandListPtr & commandList, uint8_t frame) = 0;

    virtual void beginFrame(std::chrono::time_point<std::chrono::steady_clock> now) {}
    virtual void endFrame(
        rhi::runtime::CommandListPtr & commandList, 
        rhi::runtime::CommandListPtr & transferCommandList, 
        uint8_t frame) {}

    virtual bool compare(const CustomSceneObject & object0, const CustomSceneObject & object1) const noexcept {
        return false;
    }

    inline bool isDrawEnabled() const noexcept {
        return (_flags & CustomSceneDelegateFlags::eDraw) != CustomSceneDelegateFlags::eNone;
    }

    inline bool isDispatchEnabled() const noexcept {
        return (_flags & CustomSceneDelegateFlags::eDispatch) != CustomSceneDelegateFlags::eNone;
    }

    inline CustomSceneDelegateFlags getFlags() const noexcept {
        return _flags;
    }

private:
    CustomSceneDelegateFlags _flags = CustomSceneDelegateFlags::eDraw;
};

}