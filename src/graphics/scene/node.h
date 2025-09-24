#pragma once

#include "math/math.h"

namespace engine::scene {

struct Node {
    Node * parent = nullptr;
    Node * firstChild = nullptr;
    Node * next = nullptr;

    mat4f transform{1.0F};
    mat4f worldTransform{1.0F};
    bool dirtyTransform = true;

    void setTransform(const mat4f & m) noexcept;
    bool calculateWorldTransform(bool recalculate) noexcept;

    void addChild(Node * child) noexcept;
    bool removeChild(Node * child) noexcept;
    bool removeFromParent() noexcept;
};

template <typename VisibleChecker>
void recalculateHierarchy(Node * node, bool recalculate, const VisibleChecker & v) noexcept {
    // todo: can do this without recursive call
    recalculate = node->calculateWorldTransform(recalculate);
    Node * n = node->firstChild;
    while (n) {
        if (v(n)) {
            recalculateHierarchy(n, recalculate, v);
        }
        n = n->next;
    }
}

}