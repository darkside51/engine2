#include "scene.h"

namespace engine::scene {

    Scene::Scene(debug::DebugDraw * debugDraw) : _rootNode(createNode()), _debugDraw(debugDraw) {}

    Scene::~Scene() {
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

    void Scene::recalculateNodes(const mat4f & cameraTransform) noexcept {
        recalculateHierarchy(_rootNode, false, [this, &cameraTransform](engine::scene::Node* n) noexcept { 
            if (_debugDraw) {
                _debugDraw->drawSphere(vec3f{0.0f}, 0.5f, 0xff0000ffu, 7u, cameraTransform * n->worldTransform);
            }
            return true;
        });
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {

    }
}