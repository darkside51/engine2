#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "node.h"
#include "object.h"
#include "objects_register.h"

#include <memory>
#include <unordered_map>
#include <utility>

namespace engine::scene {

struct ObjectsCollection : IntrusiveList<Object> {
    virtual ~ObjectsCollection() = default;
    virtual bool destroyObject(Object * object) noexcept { return false; }

    void emplace(Object * object) noexcept {
        IntrusiveList<Object>::emplace(object);
        dirty = true;
    }
   
public:
    bool dirty = false;
};

template<typename T>
struct ObjectsHolder : ObjectsCollection {
    using Type = T;
    using ObjectsAllocator = allocators::PoolAllocatorChain<sizeof(T), alignof(T), false>;
    ObjectsAllocator allocator{512u, 16u};

    template <typename... Args>
    Object * createObject(Args &&... args) noexcept {
        auto * object = allocator.template create<T>(std::forward<Args>(args)...);
        emplace(object);
        return object;
    }

    bool destroyObject(Object * object) noexcept override {
        remove(object);
        return allocator.destroy(object);
    }
};

class Scene {
public:
    Scene(debug::DebugDraw * debugDraw);
    ~Scene();

    Node * createNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    void recalculate(const mat4f & cameraTransform) noexcept;
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform);

    template <typename T, typename... Args>
    Object * createObject(Args &&... args) {
        constexpr auto id = ctti::typeIndex<Object, T>();
        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            auto [iter, _] = _objectCollections.emplace(id, std::pair{std::make_unique<ObjectsHolder<T>>(), Object::Iterator{}});
            it = iter;
        }

        auto & objectsData = static_cast<ObjectsHolder<T>&>(*it->second.first);
        return objectsData.createObject(std::forward<Args>(args)...);
    }

    bool destroyObject(Object * object) noexcept;

private:
    struct Layers {
        uint16_t minLayer = 0xff'ffu;
        uint16_t maxLayer = 0u;
    };
    Layers prepareObjects(ObjectsCollection & objects, size_t type) noexcept;
    Object::Iterator drawEmptyObjects(Object::Iterator it, Object::Iterator end, const mat4f & cameraTransform, uint32_t color, uint16_t layer);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false>;
    NodesAllocator _nodeAllocator{512u, 64u};
    Node * _rootNode = nullptr;

    std::unordered_map<size_t, std::pair<std::unique_ptr<ObjectsCollection>, Object::Iterator>> _objectCollections;

    debug::DebugDraw * _debugDraw = nullptr;
};

}