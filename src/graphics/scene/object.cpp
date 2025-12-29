#include "object.h"
#include "common/type_id.h"
#include "graphics/scene/register.h"

namespace engine::scene {
    Dummy::Dummy() : Object(ctti::typeIndex<Object, Dummy>()) {}
}