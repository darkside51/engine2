#include "object.h"
#include "common/type_id.h"
#include "graphics/scene/objects_register.h"

namespace engine::scene {

    EmptyObject::EmptyObject(float s) : Object(ctti::typeIndex<Object, EmptyObject>()), size(s) {}
    
}