#pragma once

#include "math/math.h"
#include "bounds.h"

#include <cassert>
#include <cstdint>

namespace engine::scene {

constexpr uint8_t kDirtyTransformBit = 0u;
constexpr uint8_t kCulledBit = 1u;
constexpr uint8_t kHiddenBit = 2u;
constexpr uint8_t kChildrenTransformsDirty = 3u;

constexpr uint8_t kDefaultState = 1u << kDirtyTransformBit | 
                                0u << kCulledBit | 0u << kHiddenBit | 
                                0u << kChildrenTransformsDirty;

struct Object;

struct Node {
    struct State {
        uint8_t state = kDefaultState;

        inline bool isCulled() const noexcept {
            return state & (1u << kCulledBit);
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

        inline void setCulled(bool culled) noexcept {
            culled ? state |= (1u << kCulledBit) : state &= ~(1u << kCulledBit);
        }

        inline void setHidden(bool hidden) noexcept {
            hidden ? state |= (1u << kHiddenBit) : state &= ~(1u << kHiddenBit);
        }

        inline void setTransformDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kDirtyTransformBit) : state &= ~(1u << kDirtyTransformBit);
        }

        inline void setChildrenTransformsDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kChildrenTransformsDirty) : state &= ~(1u << kChildrenTransformsDirty);
        }
    };

    Node() : state{kDefaultState} {}

    ~Node() {
        parent = nullptr;
        next = nullptr;
        firstChild = nullptr;
    }
    
    Node * parent = nullptr;
    Node * next = nullptr; // other childrens in parent (brothers)
    Node * firstChild = nullptr; // children begin
    Object * object = nullptr;
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

    void setObject(Object * object) noexcept;
    Object * removeObject() noexcept;
    Object * getObject() const noexcept;

    inline bool isCulled() const noexcept {
        return state.isCulled();
    }

    inline bool isHidden() const noexcept {
        return state.isHidden();
    }

    inline bool isTransformDirty() const noexcept {
        return state.isTransformDirty();
    }

    inline bool isVisible() const noexcept {
        return !state.isCulled() && !state.isHidden();
    }

    inline bool isChildrenTransformsDirty() const noexcept {
        return state.isChildrenTransformsDirty();
    }

    inline void setCulled(bool culled) noexcept {
        state.setCulled(culled);
    }
    
    inline void setHidden(bool hidden) noexcept {
        state.setHidden(hidden);
    }

    inline void setTransformDirty(bool dirty) noexcept {
        state.setTransformDirty(dirty);
    }

    inline void setChildrenTransformsDirty(bool dirty) noexcept {
        state.setChildrenTransformsDirty(dirty);
    }

    const mat4f & getTransform() noexcept {
        return transform;
    }

    const mat4f & getWorldTransform() noexcept {
        return worldTransform;
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
            node->setCulled(!v(node));
        } else {
            node->setChildrenTransformsDirty(false); // end child calculation
        }
        
        if (node->isVisible() && node->firstChild && process) {
            node = node->firstChild;
            process = true;
        } else if (node->next != nullptr) {
            node = node->next;
            process = true;
        } else {
            node = node->parent;
            process = false;
        }
    }
}


}