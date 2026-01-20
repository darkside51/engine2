#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "animation.h"
#include "bounds.h"
#include "frustum.h"
#include "node.h"
#include "object.h"
#include "register.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::scene {

constexpr bool kDebugDrawBounds = true;
constexpr uint32_t kFillNodesReserveSize = 1024u;
    
class AnimationManager;

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

namespace animation {
struct AnimatedObject {
    const AnimationInfo* animInfo = nullptr;
    Object * object = nullptr;
};
}

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
    template<typename T>
    AnimationInfoChain * createAnimInfo(uint16_t animationId, float startTime, float weight = 1.0f) {
        return _animationInfosAllocator.create<AnimationInfoChain>(startTime, weight, animationId, ctti::typeIndex<Animation, T>());
    }
    bool destroyAnimInfo(AnimationInfoChain * info) noexcept;

    // render
    uint32_t registerPipeline(rhi::runtime::PipelinePtr && pipeline);
    const rhi::runtime::PipelinePtr & getPipeline(uint32_t pipelineId) const noexcept;

    void initRenderData(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
    void newFrame(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

    IntrusiveList<Object> makeRenderList(const mat4f & cameraTransform, Node * root) noexcept;

    template <typename Condition>
    void fillRenderObjects(const mat4f & cameraTransform, Node * root, Condition && condition, std::vector<Object*> & objects);

    void update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, IntrusiveList<Object> & objects, 
        const AnimationManager & animationManager);
    void draw(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const mat4f & cameraTransform, IntrusiveList<Object> & objects);

private:
    static bool compareObjectsForDraw(const Object & a, const Object & b) noexcept;
    void prepareDrawData(IntrusiveList<Object> & objects, uint8_t frame) noexcept;

    uint32_t calculateTargetStagingSize() const noexcept;
    void performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame);
    void reduceUsedMemory(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

    // draw
    void drawObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const mat4f & cameraTransform, Object::Iterator begin, Object::Iterator end, uint16_t typeIdx);

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

    // animation
    // static thread_local std::vector<animation::AnimatedObject> _sequences; //
    // can use threads::unique_t<animation::AnimatedObject>::id() - if want to create AnimatedObjects on some threads!
    std::vector<animation::AnimatedObject> _sequences;

    // render
    rhi::runtime::Rhi * _api = nullptr;
    rhi::runtime::Device * _device = nullptr;

    std::vector<rhi::runtime::PipelinePtr> _pipelines;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

    uint32_t _spritesVertexesCount = 0u;
    uint32_t _spritesIndicesCount = 0u;

    std::chrono::time_point<std::chrono::steady_clock> _tryReduceUsedMemoryTime;
    uint32_t _lastFramesUsedSpritesVertexCount = 0u;
    uint32_t _lastFramesUsedSpritesIndicesCount = 0u;
    
    debug::DebugDraw * _debugDraw = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _spritesVertices = nullptr;
    std::unique_ptr<graphics::InflightBuffer> _spritesIndices = nullptr;

    rhi::SamplerValue _linearSampler;
};

template <typename Condition>
void Scene::fillRenderObjects(const mat4f & cameraTransform, Node * root, Condition && condition, std::vector<Object*> & objects) {
    const Frustum frustum(cameraTransform);
    objects.clear();
    objects.reserve(kFillNodesReserveSize);

    recalculateHierarchy(root, [this, &frustum, &cameraTransform, &objects, &condition](Node * n) noexcept { 
        if (n->isHidden()) {
            return false;
        }

        const auto & bounds = n->bounds;
        const auto & worldTransform = n->getWorldTransform();
        if (bounds.type != Bounds::Type::eNone) {
            if (!frustum.checkBounds(bounds.transform(worldTransform))) {
                return false;
            }
        }

        if (auto * object = n->getObject() && condition(object)) {
            objects.emplace_back(object);
        }

        if constexpr (kDebugDrawBounds) {
            if (_debugDraw) {
                const auto transform = cameraTransform * worldTransform;
                _debugDraw->drawGizmo(1.0f, transform);
                _debugDraw->drawBounds(bounds, 0xff00ffffu, transform);
            }
        }
        return true;
    });

    std::sort(objects.begin(), objects.end(), [](const Object * a, const Object * b) noexcept {
        return compareObjectsForDraw(*a, *b);
    });
}

}