#include "node.h"

namespace engine::scene {

    void Node::setTransform(const mat4f & m) noexcept {
        transform = m;
        dirtyTransform = true;
    }

    bool Node::calculateWorldTransform(bool recalculate) noexcept {
        if (dirtyTransform || recalculate) {
            if (parent == nullptr) {
                worldTransform = transform;
            } else { [[likely]]
                worldTransform = parent->worldTransform * transform;
            }
            dirtyTransform = false;
            return true;
        }

        return false;
    }

    void Node::addChild(Node * child) noexcept {
        if (child->parent) {
            // todo: log
            return;
        }

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
        if (!child->parent || !firstChild) {
            // todo: log
            return false;
        }

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
        }

        return false;
    }

}