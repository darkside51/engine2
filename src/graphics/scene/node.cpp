#include "node.h"
#include "object.h"

namespace engine::scene {

    void Node::setTransform(const mat4f & m) noexcept {
        if (isFinal()) {
            return;
        }

        data.link.transform = m;
        state.setTransformDirty(true);
    }

    bool Node::calculateWorldTransform(bool recalculate) noexcept {
        if (isFinal()) {
            return false;
        }

        if (state.isTransformDirty() || recalculate) {
            auto & link = data.link;
            if (parent == nullptr) {
                link.worldTransform = link.transform;
            } else { [[likely]]
                link.worldTransform = parent->data.link.worldTransform * link.transform;
            }
            state.setTransformDirty(false);
            return true;
        }

        return false;
    }

    void Node::addChild(Node * child) noexcept {
        if (isFinal()) {
            // todo: log
            return;
        }

        if (child->parent) {
            // todo: log
            return;
        }

        child->parent = this;

        auto & link = data.link;
        if (link.firstChild == nullptr) {
            link.firstChild = child;
            return;
        }

        Node * n = link.firstChild;
        while (n->next) {
            n = n->next;
        }
        n->next = child;
    }

    bool Node::removeChild(Node * child) noexcept {
        if (isFinal()) {
            // todo: log
            return false;
        }

        auto & link = data.link;
        if (child->parent != this || !link.firstChild) {
            // todo: log
            return false;
        }

        child->parent = nullptr;

        if (child == link.firstChild) {
            link.firstChild = child->next;
            return true;
        }

        Node * n = link.firstChild;
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

    void Node::setObject(Object * object, Final::ObjectType type) noexcept {
        if (isIntermediate()) {
            return;
        }

        data.object.object = object;
        data.object.type = type;
    }

    Object * Node::removeObject() noexcept {
        if (isIntermediate()) {
            return nullptr;
        }

        auto * result = data.object.object;
        data.object.object = nullptr;
        data.object.type = Final::ObjectType::eUnknown;
        result->matrix = nullptr;
        return result;
    }

    Object * Node::getObject() const noexcept {
        if (isIntermediate()) {
            return nullptr;
        }

        return data.object.object;
    }

    void Node::setBounds(const Bounds & newBounds) noexcept {
        bounds = newBounds;
    }

}