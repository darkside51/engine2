#pragma once

#include "math/math.h"
#include "bounds.h"

#include <cstdint>

namespace engine::scene {

constexpr uint8_t kDirtyTransformBit = 0u;
constexpr uint8_t kVisibleBit = 1u;
constexpr uint8_t kHiddenBit = 2u;
constexpr uint8_t kChildrenTransformsDirty = 3u;
constexpr uint8_t kDefaultState = 1u << kDirtyTransformBit | 1u << kVisibleBit | 0u << kHiddenBit | 0u << kChildrenTransformsDirty;

struct Node {
    struct State {
        uint8_t state = kDefaultState;

        inline bool isVisible() const noexcept {
            return state & (1u << kVisibleBit);
        }

        inline bool isHidden() const noexcept {
            return state & (1u << kHiddenBit);
        }

        inline bool isTransformDirty() const noexcept {
            return state & (1u << kDirtyTransformBit);
        }

        inline bool isChildrenTransformsDirty() const noexcept {
            return state & (1u << kChildrenTransformsDirty);
        }

        inline void setVisible(bool visible) noexcept {
            visible ? state |= (1u << kVisibleBit) : state &= ~(1u << kVisibleBit);
        };
        inline void setHidden(bool hidden) noexcept {
            hidden ? state |= (1u << kHiddenBit) : state &= ~(1u << kHiddenBit);
        };

        inline void setTransformDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kDirtyTransformBit) : state &= ~(1u << kDirtyTransformBit);
        };

        inline void setChildrenTransformsDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kChildrenTransformsDirty) : state &= ~(1u << kChildrenTransformsDirty);
        };
    };

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
    State state;

    void setTransform(const mat4f & m) noexcept;
    bool calculateWorldTransform(bool recalculate) noexcept;
    void setBounds(const Bounds & newBounds) noexcept;

    void addChild(Node * child) noexcept;
    bool removeChild(Node * child) noexcept;
    bool removeFromParent() noexcept;

    inline bool isVisible() const noexcept {
        return state.isVisible();
    }

    inline bool isHidden() const noexcept {
        return state.isHidden();
    }

    inline bool isTransformDirty() const noexcept {
        return state.isTransformDirty();
    }

    inline bool isChildrenTransformsDirty() const noexcept {
        return state.isChildrenTransformsDirty();
    }

    inline void setVisible(bool visible) noexcept {
        state.setVisible(visible);
    };
    inline void setHidden(bool hidden) noexcept {
        state.setHidden(hidden);
    };

    inline void setTransformDirty(bool dirty) noexcept {
        state.setTransformDirty(dirty);
    };

    inline void setChildrenTransformsDirty(bool dirty) noexcept {
        state.setChildrenTransformsDirty(dirty);
    }
};

// template <typename VisibleChecker>
// void recalculateHierarchyRecursive(Node * node, bool recalculate, const VisibleChecker & v) noexcept {
//     recalculate = node->calculateWorldTransform(recalculate);
//     node->setVisible(v(node));
//     if (node->isVisible() && !node->isHidden()) {
//         Node * n = node->firstChild;
//         while (n) {
//             recalculateHierarchy(n, recalculate, v);
//             n = n->next;
//         }
//     }
// }

template <typename VisibleChecker>
void recalculateHierarchy(Node * node, VisibleChecker && v, bool forceRecalculate = false) noexcept {
    bool process = true;
    while (node != nullptr) {
        if (process) {
            if (node->calculateWorldTransform(forceRecalculate || (node->parent ? node->parent->isChildrenTransformsDirty() : false))) {
                node->setChildrenTransformsDirty(true);
            }
            node->setVisible(v(node));
        } else {
            node->setChildrenTransformsDirty(false); // end child calculation
        }
        
        if (node->isVisible() && !node->isHidden() && node->firstChild && process) {
            node = node->firstChild;
            process = true;
        } else if(node->next != nullptr) {
            node = node->next;
            process = true;
        } else {
            node = node->parent;
            process = false;
        }
    }
}



}