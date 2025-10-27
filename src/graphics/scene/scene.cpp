#include "scene.h"
#include "bounds.h"
#include "frustum.h"

#include <algorithm>

namespace engine::scene {

    Scene::Scene(debug::DebugDraw * debugDraw) : _rootNode(createNode()), _debugDraw(debugDraw) {
        registerTypes();
    }

    Scene::~Scene() {
        // for (auto & [_, objects] : _objectCollections) {
        // }

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

        return it->second.first->destroyObject(object);
    }

    void Scene::recalculate(const mat4f & cameraTransform) noexcept {
        const Frustum frustum(cameraTransform);

        recalculateHierarchy(_rootNode, [this, &frustum, &cameraTransform](engine::scene::Node * n) noexcept { 
            const auto & bounds = n->bounds;
            if (bounds.type != Bounds::Type::eNone) {
                if (!frustum.checkBounds(bounds.transform(n->worldTransform))) {
                    return false;
                }
            }

            if (_debugDraw && !n->isHidden()) {
                const auto transform = cameraTransform * n->worldTransform;
                _debugDraw->drawGizmo(1.0f, transform);
                _debugDraw->drawBounds(bounds, 0xff00ffffu, transform);
            }
            return true;
        });
    }

    Scene::Layers Scene::prepareObjects(ObjectsCollection & objects, size_t type) noexcept {

        const auto layersDet2 = [](Scene::Layers & result, const Object & a, const Object & b) noexcept {
            auto const [min, max] = std::minmax(a.layer, b.layer);
            result.minLayer = std::min(result.minLayer, min);
            result.maxLayer = std::max(result.maxLayer, max);
        };

        const auto layersDet1 = [](Scene::Layers & result, const Object & a) noexcept {
            result.minLayer = std::min(result.minLayer, a.layer);
            result.maxLayer = std::max(result.maxLayer, a.layer);
        };

        Scene::Layers result{};
        switch (type) {
            case ctti::typeIndex<Object, EmptyObject>():
            {
                if (objects.dirty) {
                    // sort by order and layers if need
                    objects.stable_sort([&result, &layersDet2](const Object & a, const Object & b) noexcept {
                        layersDet2(result, a, b);
                        if (a.layer != b.layer) {
                            return a.layer < b.layer;
                        }
                        return a.node->isVisible() > b.node->isVisible();
                    });

                    objects.dirty = false;
                } else {
                    for (const auto & object : objects) {
                        layersDet1(result, object);
                    }
                }
            }
            break;
            default: 
            break;
        }

        return result;
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform) {
        Scene::Layers layers{};
        for (auto & [id, objects] : _objectCollections) {
            objects.second = objects.first->begin();
            const auto objectsLayers = prepareObjects(*objects.first, id);
            layers.minLayer = std::min(layers.minLayer, objectsLayers.minLayer);
            layers.maxLayer = std::max(layers.maxLayer, objectsLayers.maxLayer);
        }

        for (auto layer = layers.minLayer; layer <= layers.maxLayer; ++layer) {
            for (auto & [id, objects] : _objectCollections) {
                switch (id) {
                    case ctti::typeIndex<Object, EmptyObject>():
                        objects.second = drawEmptyObjects(objects.second, objects.first->end(), cameraTransform, 0xff'00'00'ffu, layer);
                    break;
                    default: 
                    break;
                }
            }
        }
    }

    Object::Iterator Scene::drawEmptyObjects(Object::Iterator it, Object::Iterator end, const mat4f & cameraTransform, 
        uint32_t color, uint16_t layer) {
        // for (const auto & object : objects) {
        for (; it != end; ++it) {
            const auto & object = *it;
            if (object.layer != layer) {
                break;
            }
            if (object.node) {
                if (object.node->isVisible() && _debugDraw) {
                _debugDraw->drawSphere(vec3f{0.0f}, static_cast<const EmptyObject&>(object).size, color, 
                    32u, cameraTransform * object.node->worldTransform);
                }
                object.node->setActive(false); //
            }
        }

        return it;
    }
}