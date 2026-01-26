#include "node.h"
#include "object.h"

#include "log/log.h"

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

    Node * Node::getChild(uint16_t nodeId) const noexcept {
        if (firstChild == nullptr) {
            return nullptr;
        }

        Node * n = firstChild;
        while (n) {
            if (n->id == nodeId) {
                return n;
            }
            n = n->next;
        }

        return nullptr;
    }

    Node * Node::findChild(uint16_t nodeId) const noexcept {
        auto * result = getChild(nodeId);
        if (result == nullptr) {
            Node * n = firstChild;
            while (n) {
                result = n->findChild(nodeId);
                if (result) {
                    break;
                }
            }
        }
        return result;
    }

    void Node::addChild(Node * child) noexcept {
        if (child->parent) {
            LOG_E("Node::addChild: child already has parent!");
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
        child->setTransformDirty(true);
    }

    bool Node::removeChild(Node * child) noexcept {
        if (child->parent != this || !firstChild) {
            LOG_E("Node::removeChild: child has no parent!");
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

    void Node::setObject(Object * object) noexcept {
        this->object = object;
        object->node = this;
    }

    Object * Node::removeObject() noexcept {
        object->node = nullptr;
        auto * result = object;
        object = nullptr;
        return result;
    }

    Object * Node::getObject() const noexcept {
        return object;
    }

    void Node::setBounds(const Bounds & newBounds) noexcept {
        bounds = newBounds;
    }

    void Node::applyTransformations() noexcept {
        if (!state.isTransformationsDirty()) {
            return;
        }

        transformation.apply(transform);

        state.setTransformationsDirty(false);
        state.setTransformDirty(true);
    }

}