#pragma once

#include "math/math.h"
#include "bounds.h"

#include <cassert>
#include <cstdint>

namespace engine::scene {

constexpr uint8_t kFinalNodeBit = 0u;
constexpr uint8_t kDirtyTransformBit = 1u;
constexpr uint8_t kCulledBit = 2u;
constexpr uint8_t kHiddenBit = 3u;
constexpr uint8_t kChildrenTransformsDirty = 4u;
constexpr uint8_t kActive = 5u; // todo: remove it

constexpr uint8_t kDefaultState = 1u << kDirtyTransformBit | 
                                0u << kCulledBit | 0u << kHiddenBit | 
                                0u << kChildrenTransformsDirty | 1u << kActive;

constexpr uint8_t kDefaultIntermediateState = 0u << kFinalNodeBit | kDefaultState;
constexpr uint8_t kDefaultFinalState = 1u << kFinalNodeBit | kDefaultState;

struct Object;

struct Node {
    struct State {
        uint8_t state = kDefaultState;

        inline bool isFinal() const noexcept {
            return state & 1u;
        }

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

        inline bool isActive() const noexcept {
            return state & (1u << kActive);
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

        inline void setActive(bool active) noexcept {
            active ? state |= (1u << kActive) : state &= ~(1u << kActive);
        }
    };

    struct Intermediate {
        Node * firstChild = nullptr; // children begin
        mat4f transform{1.0F};
        mat4f worldTransform{1.0F};
    };

    struct Final {
        enum class ObjectType : uint8_t {
            eUnknown = 0u,
            eGraphics = 1u,
            eSound = 2u
        };
        Object * object = nullptr;
        ObjectType type = ObjectType::eUnknown;
    };

    union Data {
        Intermediate link{};
        Final object;
    };

    explicit Node(bool final) : state{final ? kDefaultFinalState : kDefaultIntermediateState} {}

    ~Node() {
        parent = nullptr;
        next = nullptr;
        data.link.firstChild = nullptr;
    }
    
    Node * parent = nullptr;
    Node * next = nullptr; // other childrens in parent (brothers)
    Data data = {};
    Bounds bounds{};
    State state;

    void setTransform(const mat4f & m) noexcept;
    bool calculateWorldTransform(bool recalculate) noexcept;
    void setBounds(const Bounds & newBounds) noexcept;

    void addChild(Node * child) noexcept;
    bool removeChild(Node * child) noexcept;
    bool removeFromParent() noexcept;

    void setObject(Object * object, Final::ObjectType type = Final::ObjectType::eGraphics) noexcept;
    Object * removeObject() noexcept;
    Object * getObject() const noexcept;

    inline bool isFinal() const noexcept {
        return state.isFinal();
    }

    inline bool isIntermediate() const noexcept {
        return !state.isFinal();
    }

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
        return !state.isCulled() && !state.isHidden() && state.isActive();
    }

    inline bool isChildrenTransformsDirty() const noexcept {
        return state.isChildrenTransformsDirty();
    }

    inline bool isActive() const noexcept {
        return state.isActive();
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

    inline void setActive(bool active) noexcept {
        state.setActive(active);
    }

    const mat4f & getTransform() noexcept {
        assert(isIntermediate());
        return data.link.transform;
    }

    const mat4f & getWorldTransform() noexcept {
        const bool intermediate = isIntermediate();
        assert(parent != nullptr || intermediate);
        return intermediate ? data.link.worldTransform : parent->data.link.worldTransform;
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
        if (node->isFinal()) { // has no children
            if (process) {
                node->setCulled(!v(node));
                node->setActive(true); // node has been checked!
            }

            if (node->next != nullptr) {
                node = node->next;
                process = true;
            } else {
                node = node->parent;
                process = false;
            }
        } else {
            if (process) {
                if (node->calculateWorldTransform(forceRecalculate || (node->parent ? node->parent->isChildrenTransformsDirty() : false))) {
                    node->setChildrenTransformsDirty(true);
                }
                node->setCulled(!v(node));
                node->setActive(true); // node has been checked!
            } else {
                node->setChildrenTransformsDirty(false); // end child calculation
            }
            
            if (node->isVisible() && node->data.link.firstChild && process) {
                node = node->data.link.firstChild;
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



}