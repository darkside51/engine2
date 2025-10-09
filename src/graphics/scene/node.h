#pragma once

#include "math/math.h"
#include "bounds.h"

namespace engine::scene {

struct Node {
    ~Node() {
        parent = nullptr;
        firstChild = nullptr;
        next = nullptr;
    }
    
    Node * parent = nullptr;
    Node * firstChild = nullptr;
    Node * next = nullptr; // other childrens in parent (brothers)

    mat4f transform{1.0F};
    mat4f worldTransform{1.0F};
    Bounds bounds{};
    bool dirtyTransform = true;

    void setTransform(const mat4f & m) noexcept;
    bool calculateWorldTransform(bool recalculate) noexcept;
    void setBounds(const Bounds & newBounds) noexcept;

    void addChild(Node * child) noexcept;
    bool removeChild(Node * child) noexcept;
    bool removeFromParent() noexcept;
};

template <typename VisibleChecker>
void recalculateHierarchy(Node * node, bool recalculate, const VisibleChecker & v) noexcept {
    // todo: can do this without recursive call
    recalculate = node->calculateWorldTransform(recalculate);
    if (v(node)) {
        Node * n = node->firstChild;
        while (n) {
            recalculateHierarchy(n, recalculate, v);
            n = n->next;
        }
    }
}

}