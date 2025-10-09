#include "object.h"
#include "common/type_id.h"

namespace engine::scene {

    EmptyObject::EmptyObject(float s, Node * node) : Object(ctti::typeIndex<Object, EmptyObject>(), node), size(s) {
        node->setBounds(Bounds::createSphere(vec3f{0.0f}, size));
    }

}