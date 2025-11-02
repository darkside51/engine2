#pragma once

#include "math/math.h"
#include "utils/intrusive_list.h"

#include <cstdint>

namespace engine::scene {

    struct Object : IntrusiveListNode<Object> {
        using Iterator = IntrusiveList<Object>::Iterator;
        const mat4f * matrix = nullptr;
       
        uint16_t typeIndex = 0u;
        // uint16_t layer = 0u;
        uint16_t order = 0u;

        Object(uint16_t typeIndex) : typeIndex(typeIndex) {}
        virtual ~Object() = default;

        // void setLayer(uint16_t l) noexcept { layer = l; }
        // uint16_t getLayer() const noexcept { return layer; }

        void setOrder(uint16_t o) noexcept { order = o; }
        uint16_t getOrder() const noexcept { return order; }
        
        bool cmp(const Object &) const noexcept { return false; }
    };

    struct EmptyObject : Object {
        float size = 0.0f;
        explicit EmptyObject(float s);
        ~EmptyObject() = default;
    };

}