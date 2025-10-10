#pragma once

#include "node.h"
#include "math/math.h"

#include <cstdint>

namespace engine::scene {

    struct Object {
        Node * node = nullptr;
        Object *next = nullptr;
        Object *prev = nullptr;
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