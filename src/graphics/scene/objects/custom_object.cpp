#include "custom_object.h"

#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

namespace engine::scene {

CustomSceneObject::CustomSceneObject(ICustomObjectSceneDelegate & delegate, void * data) : 
    AttachmentObject(ctti::typeIndex<AttachmentObject, CustomSceneObject>()), 
    _delegate(delegate),
    _customData(data) { }

bool CustomSceneObject::cmp(const CustomSceneObject & other) const noexcept {
    if (&_delegate != &other._delegate) {
        return std::less<const ICustomObjectSceneDelegate *>{}(&_delegate, &other._delegate);
    }

    return _delegate.compare(*this, other);
}

}