#pragma once

#include "math/math.h"
#include "bounds.h"

#include <cstdint>

namespace engine::scene {

constexpr uint8_t kDirtyTransformBit = 0u;
constexpr uint8_t kVisibleBit = 1u;
constexpr uint8_t kHiddenBit = 2u;
constexpr uint8_t kDefaultState = 1u << kDirtyTransformBit | 1u << kVisibleBit | 0u << kHiddenBit;

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

        inline void setVisible(bool visible) noexcept {
            visible ? state |= (1u << kVisibleBit) : state &= ~(1u << kVisibleBit);
        };
        inline void setHidden(bool hidden) noexcept {
            hidden ? state |= (1u << kHiddenBit) : state &= ~(1u << kHiddenBit);
        };

        inline void setTransformDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kDirtyTransformBit) : state &= ~(1u << kDirtyTransformBit);
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

    inline void setVisible(bool visible) noexcept {
        state.setVisible(visible);
    };
    inline void setHidden(bool hidden) noexcept {
        state.setHidden(hidden);
    };

    inline void setTransformDirty(bool dirty) noexcept {
        state.setTransformDirty(dirty);
    };
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