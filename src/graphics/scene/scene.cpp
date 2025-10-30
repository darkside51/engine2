#include "scene.h"
#include "bounds.h"
#include "frustum.h"

#include <algorithm>

namespace engine::scene {

    Scene::Scene(debug::DebugDraw * debugDraw) : _rootNode(createNode()), _debugDraw(debugDraw) {
        registerTypes();
    }

    Scene::~Scene() {
        destroyNode(_rootNode);

        // for (auto & [_, objects] : _objectCollections) {
        // }
    }

    Node * Scene::createNode() noexcept {
        return _nodeAllocator.create<engine::scene::Node>(false);
    }

    Node * Scene::createFinalNode() noexcept {
        return _nodeAllocator.create<engine::scene::Node>(true);
    }

    void Scene::destroyNode(Node * node) noexcept {
        if (node->removeFromParent() || node == _rootNode) {
            if (Node * n = node->isIntermediate() ? node->data.link.firstChild : nullptr) {
                while (n->next) {
                    auto * n1 = n->next;
                    destroyNode(n);
                    n = n1;
                }
                destroyNode(n);
            }
            if (node->isFinal()) {
                if (auto * object = node->getObject()) {
                    destroyObject(object);
                }
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

    IntrusiveList<Object> Scene::recalculate(const mat4f & cameraTransform) noexcept {
        const Frustum frustum(cameraTransform);
        IntrusiveList<Object> objects;

        recalculateHierarchy(_rootNode, [this, &frustum, &cameraTransform, &objects](Node * n) noexcept { 
            if (n->isHidden()) {
                return false;
            }

            const auto & bounds = n->bounds;
            const auto & worldTransform =  n->getWorldTransform();
            if (bounds.type != Bounds::Type::eNone) {
                if (!frustum.checkBounds(bounds.transform(worldTransform))) {
                    return false;
                }
            }

            if (n->isFinal()) {
                if (auto * object = n->getObject()) {
                    object->matrix = &n->getWorldTransform();
                    objects.emplace(object);
                }
            }

            if (_debugDraw) {
                const auto transform = cameraTransform * worldTransform;
                _debugDraw->drawGizmo(1.0f, transform);
                _debugDraw->drawBounds(bounds, 0xff00ffffu, transform);
            }
            return true;
        });

        return objects;
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform, IntrusiveList<Object> & objects) {
        if (objects.empty()) {
            return;
        }

        objects.stable_sort([](const Object & a, const Object & b) noexcept {
            if (a.layer != b.layer) {
                return a.layer < b.layer;
            }

            // todo special sorts for some types
            return a.typeIndex < b.typeIndex;
        });

        auto currentIt = objects.begin();
        const auto backIt = objects.back();
        uint16_t typeIdx = currentIt->typeIndex;
        for (auto it = currentIt; it != objects.end(); ++it) {
            bool const last = (it == backIt);
            if (it->typeIndex != typeIdx || last) {
                switch (typeIdx) {
                case ctti::typeIndex<Object, EmptyObject>():
                {
                    drawEmptyObjects(currentIt, (last ? objects.end(): it), cameraTransform, 0xff'00'00'ffu);
                }
                    break;
                default:
                    break;
                }

                currentIt = it;
                typeIdx = it->typeIndex;
            }
        }
    }

    void Scene::drawEmptyObjects(Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform, uint32_t color) {
        if (_debugDraw == nullptr) {
            return;
        }

        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _debugDraw->drawSphere(vec3f{0.0f}, static_cast<const EmptyObject&>(object).size, color, 
                32u, cameraTransform * (*object.matrix));
        }
    }
}