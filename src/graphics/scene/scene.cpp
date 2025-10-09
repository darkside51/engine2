#include "scene.h"
#include "bounds.h"

#include <algorithm>

namespace engine::scene {

    Scene::~Scene() {
        for (auto & [_, objects] : _objectCollections) {
            delete objects;
        }

        destroyNode(_rootNode);
    }

    Node * Scene::createNode() noexcept {
        return _nodeAllocator.create<engine::scene::Node>();
    }

    void Scene::destroyNode(Node * node) noexcept {
        if (node->removeFromParent() || node == _rootNode) {
            if (Node * n = node->firstChild) {
                while (n->next) {
                    auto * n1 = n->next;
                    destroyNode(n);
                    n = n1;
                }
                destroyNode(n);
            }
            _nodeAllocator.destroy(node);
        }
    }

    bool Scene::destroyObject(Object * object) noexcept {
        const auto id = object->typeIndex;

        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            return false;
        }

        return it->second->destroyObject(object);
    }

    void Scene::recalculateNodes(const mat4f & cameraTransform) noexcept {
        recalculateHierarchy(_rootNode, false, [this, &cameraTransform](engine::scene::Node* n) noexcept { 
            if (!n->isVisible() || n->isHidden()) {
                return false;
            }

            if (_debugDraw) {
                const auto transform = cameraTransform * n->worldTransform;
                _debugDraw->drawGizmo(1.0f, transform);
                // _debugDraw->drawCube(vec3f{-0.1f}, vec3f{0.1f}, 0xff00ffffu, transform);
                _debugDraw->drawBounds(n->bounds, 0xff00ffffu, transform);
            }
            return true;
        });
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform) {
        for (const auto & [id, objects] : _objectCollections) {
            switch (id) {
                case ctti::typeIndex<Object, EmptyObject>():
                    drawEmptyObjects(*objects, cameraTransform, 0xffffffffu);
                break;
                default: 
                break;
            }
        }
    }

    void Scene::drawEmptyObjects(const ObjectsCollection & objects, const mat4f & cameraTransform, uint32_t color) {
        for (const auto & object : objects) {
            if (object.node && _debugDraw) {
                _debugDraw->drawSphere(vec3f{0.0f}, static_cast<const EmptyObject&>(object).size, color, 
                    32u, cameraTransform * object.node->worldTransform);
            }
        }
    }
}