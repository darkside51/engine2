#pragma once

#include "math/math.h"
#include "bounds.h"
#include "transform.h"

#include <cassert>
#include <cstdint>

namespace engine::scene {

constexpr uint8_t kDirtyTransformBit = 0u;
constexpr uint8_t kCulledBit = 1u;
constexpr uint8_t kHiddenBit = 2u;
constexpr uint8_t kChildrenTransformsDirty = 3u;
constexpr uint8_t kTransformationsDirty = 4u;

constexpr uint8_t kDefaultState = 1u << kDirtyTransformBit | 
                                0u << kCulledBit | 0u << kHiddenBit | 
                                0u << kChildrenTransformsDirty | 0u << kTransformationsDirty;

struct AttachmentObject;

struct Node {
    inline static constexpr uint16_t kUnknownId = 0xff'ff;
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

        inline bool isTransformationsDirty() const noexcept {
            return state & (1u << kTransformationsDirty);
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

        inline void setTransformationsDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kTransformationsDirty) : state &= ~(1u << kTransformationsDirty);
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
    AttachmentObject * attachment = nullptr;
    mat4f transform{1.0F};
    mat4f worldTransform{1.0F};
    Transformation transformation{};
    Bounds bounds{};
    uint16_t id = kUnknownId;
    State state{kDefaultState};

    uint16_t getId() const noexcept { return id; }
    void setId(uint16_t nodeId) noexcept { id = nodeId; }

    void setTransform(const mat4f & m) noexcept;
    bool calculateWorldTransform(bool recalculate) noexcept;
    void setBounds(const Bounds & newBounds) noexcept;

    // attachment
    void setAttachment(AttachmentObject * object) noexcept;
    AttachmentObject * removeAttachment() noexcept;
    AttachmentObject * getAttachment() const noexcept;

    // state
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

    const mat4f & getTransform() const noexcept {
        return transform;
    }

    const mat4f & getWorldTransform() const noexcept {
        return worldTransform;
    }

    // children
    void addChild(Node * child) noexcept;
    bool removeChild(Node * child) noexcept;
    bool removeFromParent() noexcept;

    Node * getChild(uint16_t nodeId) const noexcept;
    Node * findChild(uint16_t nodeId) const noexcept;

    // transformation
    quatf getRotation() const noexcept { return transformation.getRotation(); }
    vec3f getRotationAngles() const noexcept { return transformation.getRotationAngles(); }
    vec3f getScale() const noexcept { return transformation.getScale(); }
    vec3f getPosition() const noexcept { return transformation.getPosition(); }

    void setPosition(const vec3f & p) noexcept {
        if (transformation.setPosition(p)) {
            state.setTransformationsDirty(true);
        }
    }

    template <typename T>
    void setScale(T && s) noexcept {
        if (transformation.setScale(s)) {
            state.setTransformationsDirty(true);
        }
    }

    template <typename T>
    void setRotation(T && r) noexcept {
        if (transformation.setRotation(r)) {
            state.setTransformationsDirty(true);
        }
    }

    void applyTransformations() noexcept;
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

template <typename Function>
void childrenPass(Node * root, Function && f) noexcept {
    Node * node = root->firstChild;
    while (node != nullptr) {
        f(node);
        node = node->next;
    }
}

template <typename Function>
void hierarchyPass(Node * root, Function && f) noexcept {
    Node * node = root;
    bool process = true;
    while (node != nullptr) {
        if (process) {
            f(node);
        }
        
        if (node->isVisible() && node->firstChild && process) {
            node = node->firstChild;
            process = true;
        } else if (node->next != nullptr && node != root) {
            node = node->next;
            process = true;
        } else {
            node = node->parent;
            process = false;
            if (node == root) {
                break;
            }
        }
    }
}

template <typename VisibleChecker>
void recalculateHierarchy(Node * root, VisibleChecker && v, bool forceRecalculate = false) noexcept {
    Node * node = root;
    bool process = true;
    while (node != nullptr) {
        if (process && !node->isHidden()) {
            node->applyTransformations();
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
        } else if (node->next != nullptr && node != root) {
            node = node->next;
            process = true;
        } else {
            node = node->parent;
            process = false;
            if (node == root) {
                break;
            }
        }
    }
}


}