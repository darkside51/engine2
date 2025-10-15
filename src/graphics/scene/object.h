#pragma once

#include "node.h"
#include "math/math.h"
#include "utils/intrusive_list.h"

#include <cstdint>

namespace engine::scene {

    struct Object : IntrusiveListNode<Object> {
        using Iterator = IntrusiveList<Object>::Iterator;
        Node * node = nullptr;
       
        uint16_t typeIndex = 0u;
        uint16_t layer = 0u;

        Object(uint16_t typeIndex) : typeIndex(typeIndex) {}
        Object(uint16_t typeIndex, Node * node) : typeIndex(typeIndex), node(node) {}

        void setLayer(uint16_t l) noexcept { layer = l; }
        uint16_t getLayer() const noexcept { return layer; }
    };

    struct EmptyObject : Object {
        float size = 0.0f;
        EmptyObject(float s, Node * node);
    };

}