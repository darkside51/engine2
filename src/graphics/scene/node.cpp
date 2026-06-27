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

    Node * Node::getChild(uint32_t nodeId) const noexcept {
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

    Node * Node::findChildInHierarchy(uint32_t nodeId) const noexcept {
        auto * result = getChild(nodeId);
        if (result == nullptr) {
            Node * n = firstChild;
            while (n) {
                result = n->findChildInHierarchy(nodeId);
                if (result) {
                    break;
                }
            }
        }
        return result;
    }

    Node * Node::childAt(uint32_t index) const noexcept {
        auto * result = firstChild;
        uint32_t current = 0u;
        while (result && current < index) {
            result = result->next;
            ++current;
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
                child->next = nullptr;
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

    void Node::setAttachment(AttachmentObject * object) noexcept {
        auto ** targetAttachment = &attachment;
        while (*targetAttachment) {
            targetAttachment = &(*targetAttachment)->next;
        }
        *targetAttachment = object;
        (*targetAttachment)->node = this;
    }

    AttachmentObject * Node::removeAttachment() noexcept {
        attachment->node = nullptr;
        auto * result = attachment;
        attachment = nullptr;
        return result;
    }

    AttachmentObject * Node::getAttachment() const noexcept {
        return attachment;
    }

    AttachmentObject * Node::getAttachment(uint8_t idx) const noexcept {
        auto * result = attachment;
        uint8_t current = 0u;
        while (result && current < idx) {
            result = result->next;
            ++current;
        }
        return result;
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