#pragma once

#include "node.h"
#include "math/math.h"

namespace engine::scene {

    struct Object {
        Node * node = nullptr;
        Object *next = nullptr;
        Object *prev = nullptr;
        size_t typeIndex = 0u;

        Object(size_t typeIndex) : typeIndex(typeIndex) {}
        Object(size_t typeIndex, Node * node) : typeIndex(typeIndex), node(node) {}
    };

    struct EmptyObject : Object {
        float size = 0.0f;
        EmptyObject(float s, Node * node);
    };

}