#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "node.h"
#include "object.h"
#include "objects_register.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::scene {
    
struct ObjectsCollection {
    virtual ~ObjectsCollection() = default;
    virtual bool destroyObject(Object * object) noexcept { return false; }
};

template<typename T>
struct ObjectsHolder : ObjectsCollection {
    using Type = T;
    using ObjectsAllocator = allocators::PoolAllocatorChain<sizeof(T), alignof(T), false, false>;
    ObjectsAllocator allocator{512u, 16u};

    template <typename... Args>
    Object * createObject(Args &&... args) noexcept {
        return allocator.template create<T>(std::forward<Args>(args)...);
    }

    bool destroyObject(Object * object) noexcept override {
        return allocator.destroy(object);
    }
};

class Scene {
public:
    Scene(debug::DebugDraw * debugDraw);
    ~Scene();

    Node * createNode() noexcept;
    Node * createFinalNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    void newFrame() noexcept;
    IntrusiveList<Object> recalculate(const mat4f & cameraTransform) noexcept;
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform, IntrusiveList<Object> & objects);

    template <typename T, typename... Args>
    Object * createObject(Args &&... args) {
        constexpr auto id = ctti::typeIndex<Object, T>();
        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            auto [iter, _] = _objectCollections.emplace(id, std::make_unique<ObjectsHolder<T>>());
            it = iter;
        }

        auto & objectsData = static_cast<ObjectsHolder<T>&>(*it->second);
        return objectsData.createObject(std::forward<Args>(args)...);
    }

    bool destroyObject(Object * object) noexcept;

    uint32_t registerPipeline(rhi::runtime::PipelinePtr && pipeline);
    const rhi::runtime::PipelinePtr & getPipeline(uint32_t pipelineId) const noexcept;

private:
    bool compareObjects(const Object & a, const Object & b) const noexcept;

    void drawEmptyObjects(Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform, uint32_t color);

    void drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform);
    void drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, Object::Iterator begin, Object::Iterator end, 
        const mat4f & cameraTransform, uint32_t batchVertexesCount, uint32_t batchIndicesCount);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false, false>;
    NodesAllocator _nodeAllocator{512u, 64u};
    Node * _rootNode = nullptr;

    std::unordered_map<size_t, std::unique_ptr<ObjectsCollection>> _objectCollections;

    debug::DebugDraw * _debugDraw = nullptr;

    std::vector<rhi::runtime::PipelinePtr> _pipelines;

    uint32_t _spritesVertexesCount = 0u;
    uint32_t _spritesIndicesCount = 0u;
    std::unique_ptr<graphics::InflightBuffer> _spritesVertices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _spritesIndices = nullptr;
};

}