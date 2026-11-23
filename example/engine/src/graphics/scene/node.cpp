#include "node.h"
#include "object.h"

#include "log/log.h"

namespace engine::scene {

    void Node::setTransform(const mat4f & m) noexcept {
        vec3f scale{1.0f};
        quatf rotation{1.0f, 0.0f, 0.0f, 0.0f};
        vec3f translation{0.0f};
        vec3f skew{0.0f};
        vec4f perspective{0.0f};

        if (decompose(m, scale, rotation, translation, skew, perspective)) {
            transforms->scale[slot] = scale;
            transforms->rotation[slot] = rotation;
            transforms->position[slot] = translation;
            transforms->pivot[slot] = vec3f{0.0f};
        } else {
            transforms->scale[slot] = vec3f{1.0f};
            transforms->rotation[slot] = quatf{1.0f, 0.0f, 0.0f, 0.0f};
            transforms->position[slot] = vec3f{m[3u]};
            transforms->pivot[slot] = vec3f{0.0f};
        }

        state.setTransformationsDirty(true);
        state.setTransformDirty(true);
    }

    void Node::applyTransformations() noexcept {
        if (!state.isTransformationsDirty()) {
            return;
        }

        // rebuild the cached local matrix from TRS (single source of truth)
        mat4f & local = transforms->local[slot];
        local = mat4_from_quat(transforms->rotation[slot]);
        scale_matrix(local, transforms->scale[slot]);
        translate_matrix_to(local, transforms->position[slot] - mat3f(local) * transforms->pivot[slot]);

        state.setTransformationsDirty(false);
        state.setTransformDirty(true);
    }

    bool Node::calculateWorldTransform(bool recalculate) noexcept {
        if (state.isTransformDirty() || recalculate) {
            if (parent == nullptr) {
                transforms->world[slot] = transforms->local[slot];
            } else { [[likely]]
                transforms->world[slot] = parent->getWorldTransform() * transforms->local[slot];
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
        if (child == nullptr || child->parent) {
            LOG_E("Node::addChild: child already has parent!");
            return;
        }

        // a node must not keep a link to someone else's sibling list
        assert(child->next == nullptr);
        child->next = nullptr;

        child->parent = this;
        child->setTransformDirty(true);

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
        if (child == nullptr || child->parent != this || firstChild == nullptr) {
            LOG_E("Node::removeChild: child has no parent!");
            return false;
        }

        if (child == firstChild) {
            firstChild = child->next;
        } else {
            Node * n = firstChild;
            while (n->next != nullptr && n->next != child) {
                n = n->next;
            }

            if (n->next != child) {
                LOG_E("Node::removeChild: child is not in the children list!");
                return false;
            }

            n->next = child->next;
        }

        child->next = nullptr;
        child->parent = nullptr;
        child->setTransformDirty(true);

        return true;
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

}
