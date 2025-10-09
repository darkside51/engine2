#include "node.h"

namespace engine::scene {

    void Node::setTransform(const mat4f & m) noexcept {
        transform = m;
        state.setTransformDirty(true);
    }

    bool Node::calculateWorldTransform(bool recalculate) noexcept {
        if (state.isTransformDirty() || recalculate) {
            if (parent == nullptr) {
                worldTransform = transform;
            } else { [[likely]]
                worldTransform = parent->worldTransform * transform;
            }
            state.setTransformDirty(false);
            return true;
        }

        return false;
    }

    void Node::addChild(Node * child) noexcept {
        if (child->parent) {
            // todo: log
            return;
        }

        child->parent = this;

        if (firstChild == nullptr) {
            firstChild = child;
            return;
        }

        Node * n = firstChild;
        while (n->next) {
            n = n->next;
        }
        n->next = child;
    }

    bool Node::removeChild(Node * child) noexcept {
        if (!child->parent || child->parent != this || !firstChild) {
            // todo: log
            return false;
        }

        child->parent = nullptr;

        if (child == firstChild) {
            firstChild = child->next;
            return true;
        }

        Node * n = firstChild;
        while (n->next) {
            if (n->next == child) {
                n->next = child->next;
                return true;
            }
            n = n->next;
        }

        return false;
    }

    bool Node::removeFromParent() noexcept {
        if (parent == nullptr) return false;
        return parent->removeChild(this);
    }

    void Node::setBounds(const Bounds & newBounds) noexcept {
        bounds = newBounds;
    }

}