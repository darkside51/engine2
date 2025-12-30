#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "animation.h"
#include "node.h"
#include "object.h"
#include "register.h"

#include <cassert>
#include <chrono>
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
    ObjectsAllocator allocator{512u, 32u}; // 16'384 objects max

    template <typename... Args>
    Object * createObject(Args &&... args) noexcept {
        return allocator.template create<T>(std::forward<Args>(args)...);
    }

    bool destroyObject(Object * object) noexcept override {
        constexpr auto typeIndex = ctti::typeIndex<Object, T>();
        assert(object->typeIndex == typeIndex);
        return allocator.destroy(static_cast<T*>(object));
    }
};

// global
void attachObjectToNode(Object & object, Node & node) noexcept;

class Scene {
public:
    Scene(debug::DebugDraw * debugDraw);
    ~Scene();

    void destroy() noexcept;

    // nodes
    Node * createNode() noexcept;
    Node * createFinalNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    // objects
    template <typename T, typename... Args>
    T * createObject(Args &&... args) {
        constexpr auto id = ctti::typeIndex<Object, T>();
        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            auto [iter, _] = _objectCollections.emplace(id, std::make_unique<ObjectsHolder<T>>());
            it = iter;
        }

        auto & objectsData = static_cast<ObjectsHolder<T>&>(*it->second);
        return static_cast<T*>(objectsData.createObject(std::forward<Args>(args)...));
    }

    bool destroyObject(Object * object) noexcept;

    // anims
    AnimationInfoChain * createAnimInfo(uint16_t animationId, uint16_t animationType, float startTime, float weight = 1.0f);
    bool destroyAnimInfo(AnimationInfoChain * info) noexcept;

    // render
    uint32_t registerPipeline(rhi::runtime::PipelinePtr && pipeline);
    const rhi::runtime::PipelinePtr & getPipeline(uint32_t pipelineId) const noexcept;

    void initRenderData(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
    void newFrame() noexcept;

    IntrusiveList<Object> makeRenderList(const mat4f & cameraTransform, Node * root) noexcept;
    void update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, IntrusiveList<Object> & objects, const Animation & tmpAnimTest);
    void draw(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const mat4f & cameraTransform, IntrusiveList<Object> & objects);

private:
    void performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame);

    bool compareObjectsForDraw(const Object & a, const Object & b) const noexcept;

    void drawDummies(Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform, uint32_t color);

    void drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform);
    void drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, Object::Iterator begin, Object::Iterator end, 
        const mat4f & cameraTransform, uint32_t batchVertexesCount, uint32_t batchIndicesCount);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false, false>;
    NodesAllocator _nodeAllocator{512u, 64u};

    using AnimationInfosAllocator = allocators::PoolAllocatorChain<sizeof(AnimationInfoChain), alignof(AnimationInfoChain), false, false>;
    AnimationInfosAllocator _animationInfosAllocator{512u, 64u};

    Node * _rootNode = nullptr;

    std::unordered_map<size_t, std::unique_ptr<ObjectsCollection>> _objectCollections;

    debug::DebugDraw * _debugDraw = nullptr;

    // render
    rhi::runtime::Rhi * _api = nullptr;
    rhi::runtime::Device * _device = nullptr;

    std::vector<rhi::runtime::PipelinePtr> _pipelines;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

    uint32_t _spritesVertexesCount = 0u;
    uint32_t _spritesIndicesCount = 0u;

    uint32_t _framesToTryReduceUsedMemory = 0u;
    uint32_t _lastFramesUsedSpritesVertexCount = 0u;
    uint32_t _lastFramesUsedSpritesIndicesCount = 0u;
    
    std::unique_ptr<graphics::InflightBuffer> _spritesVertices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _spritesIndices = nullptr;

    rhi::SamplerValue _linearSampler;
};

}