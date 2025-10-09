#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "node.h"
#include "object.h"
#include "objects_register.h"

#include <unordered_map>

namespace engine::scene {

struct ObjectsCollection {
    virtual ~ObjectsCollection() = default;
    virtual bool destroyObject(Object * object) noexcept = 0;

    Object * rootObject = nullptr;
    Object * currentObject = nullptr;
};

template<typename T>
struct ObjectsHolder : ObjectsCollection {
    using Type = T;
    using ObjectsAllocator = allocators::PoolAllocatorChain<sizeof(T), alignof(T), false>;
    ObjectsAllocator allocator{512u, 64u};

    bool destroyObject(Object * object) noexcept override {
        if (object->prev) {
            object->prev->next = object->next;
        }
        if (object->next) {
            object->next->prev = object->prev;
        }
        if (currentObject == object) {
            currentObject = object->prev ? object->prev : object->next;
        }
        if (rootObject == object) {
            rootObject = object->next;
        }
        return allocator.destroy(object);
    }
};

class Scene {
public:
    Scene(debug::DebugDraw * debugDraw) : _rootNode(createNode()), _debugDraw(debugDraw) {
        registerTypes();
    }

    ~Scene();

    Node * createNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    void recalculateNodes(const mat4f & cameraTransform) noexcept;
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform);

    template <typename T, typename... Args>
    Object * createObject(Args &&... args) {
        constexpr auto id = ctti::typeIndex<Object, T>();
        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            auto [iter, _] = _objectCollections.emplace(id, new ObjectsHolder<T>{});
            it = iter;
        }

        auto & objectsData = *static_cast<ObjectsHolder<T>*>(it->second);

        auto * object = objectsData.allocator.template create<T>(std::forward<Args>(args)...);
        if (objectsData.rootObject == nullptr) {
            objectsData.rootObject = object;
        } else {
            object->prev = objectsData.currentObject;
            objectsData.currentObject->next = object;
        }
        objectsData.currentObject = object;

        return object;
    }

    bool destroyObject(Object * object) noexcept;

private:
    void drawEmptyObjects(Object * object, const mat4f & cameraTransform, uint32_t color);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false>;
    NodesAllocator _nodeAllocator{512u, 64u};
    Node * _rootNode = nullptr;

    std::unordered_map<size_t, ObjectsCollection*> _objectCollections;

    debug::DebugDraw * _debugDraw = nullptr;
};

}