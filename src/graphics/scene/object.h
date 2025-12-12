#pragma once

#include "math/math.h"
#include "utils/intrusive_list.h"

#include <cstdint>

namespace engine::scene {

    struct Node;

    struct Object : IntrusiveListNode<Object> {
        using Iterator = IntrusiveList<Object>::Iterator;
        Node * node = nullptr;
       
        uint16_t typeIndex = 0u;
        // uint16_t layer = 0u;
        int16_t order = 0u;

        Object(uint16_t typeIndex) : typeIndex(typeIndex) {}
        virtual ~Object() = default;

        // void setLayer(uint16_t l) noexcept { layer = l; }
        // uint16_t getLayer() const noexcept { return layer; }

        void setOrder(int16_t o) noexcept { order = o; }
        int16_t getOrder() const noexcept { return order; }
        
        bool cmp(const Object &) const noexcept { return false; }
    };

    struct Dummy : Object {
        explicit Dummy();
        ~Dummy() override = default;
    };

}