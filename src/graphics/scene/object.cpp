#include "object.h"
#include "common/type_id.h"

namespace engine::scene {

    EmptyObject::EmptyObject(float s) : Object(ctti::typeIndex<Object, EmptyObject>()), size(s) {}
    
}