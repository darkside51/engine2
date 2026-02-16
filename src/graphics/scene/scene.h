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
#include "threads/threads.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine {

namespace graphics {
    class GraphicsService;
}

namespace scene {
constexpr bool kDebugDrawBounds = false;
constexpr bool kDebugDrawGizmos = false;

constexpr uint32_t kFillNodesReserveSize = 1024u;
constexpr uint32_t kUnknownTransformIndex = 0xff'ff'ff'ffu;
    
class AnimationManager;

struct ObjectsCollection {
    virtual ~ObjectsCollection() = default;
    virtual bool destroyObject(AttachmentObject * object) noexcept { return false; }
};

template<typename T>
struct ObjectsHolder : ObjectsCollection {
    using Type = T;
    using ObjectsAllocator = allocators::PoolAllocatorChain<sizeof(T), alignof(T), false, false>;
    ObjectsAllocator allocator{512u, 256u}; // 131'072 objects max

    template <typename... Args>
    AttachmentObject * createObject(Args &&... args) noexcept {
        return allocator.template create<T>(std::forward<Args>(args)...);
    }

    bool destroyObject(AttachmentObject * object) noexcept override {
        constexpr auto typeIndex = ctti::typeIndex<AttachmentObject, T>();
        assert(object->typeIndex == typeIndex);
        return allocator.destroy(static_cast<T*>(object));
    }
};

namespace animation {
struct AnimatedObject {
    const AnimationInfo* animInfo = nullptr;
    AttachmentObject * object = nullptr;
};
}

// global
void attachObjectToNode(AttachmentObject & object, Node & node) noexcept;

class Scene {
    using AttachmentsIterator = std::span<AttachmentObject*>::iterator;
    
public:
    Scene(const graphics::GraphicsService & graphicsService, debug::DebugDraw & drawHelper);
    ~Scene();

    void destroy() noexcept;

    // nodes
    Node * createNode() noexcept;
    Node * createFinalNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    // attachments
    template <typename T, typename... Args>
    T * createObject(Args &&... args) {
        constexpr auto id = ctti::typeIndex<AttachmentObject, T>();
        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            auto [iter, _] = _objectCollections.emplace(id, std::make_unique<ObjectsHolder<T>>());
            it = iter;
        }

        auto & objectsData = static_cast<ObjectsHolder<T>&>(*it->second);
        return static_cast<T*>(objectsData.createObject(std::forward<Args>(args)...));
    }

    bool destroyObject(AttachmentObject * object) noexcept;

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
    void beginFrame(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;
    void endFrame(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame);

    template <typename Condition>
    void fillAttachmentsArray(const mat4f & cameraTransform, Node * root, Condition && condition, std::vector<AttachmentObject*> & objects);

    void update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, std::span<AttachmentObject*> objects, 
        const AnimationManager & animationManager);
    void draw(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const mat4f & cameraTransform, std::span<AttachmentObject*> objects);

private:
    static bool compareAttachmentsForDraw(const AttachmentObject & a, const AttachmentObject & b) noexcept;
    void prepareDrawData(std::span<AttachmentObject*> objects, uint8_t frame) noexcept;

    uint32_t calculateTargetStagingSize() const noexcept;
    void performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame);
    void reduceUsedMemory(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

    // draw
    void drawObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const mat4f & cameraTransform, AttachmentsIterator begin, AttachmentsIterator end, uint16_t typeIdx);

    void drawDummies(AttachmentsIterator begin, AttachmentsIterator end, const mat4f & cameraTransform, uint32_t color);
    void drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, const mat4f & cameraTransform);
    void drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const mat4f & cameraTransform, uint32_t batchVertexesCount, uint32_t batchIndicesCount);

    void drawMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, const mat4f & cameraTransform);
    void drawSameMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const mat4f & cameraTransform, uint32_t count, bool skeletal);   

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false, false>;
    NodesAllocator _nodeAllocator{512u, 256u};

    using AnimationInfosAllocator = allocators::PoolAllocatorChain<sizeof(AnimationInfoChain), alignof(AnimationInfoChain), false, false>;
    AnimationInfosAllocator _animationInfosAllocator{512u, 256u};

    Node * _rootNode = nullptr;

    std::unordered_map<size_t, std::unique_ptr<ObjectsCollection>> _objectCollections;

    // animation
    // static thread_local std::vector<animation::AnimatedObject> _sequences; //
    // can use threads::unique_t<animation::AnimatedObject>::id() - if want to create AnimatedObjects on some threads!
    struct AnimationCollection {
        std::vector<animation::AnimatedObject> skeletalAnimations;
        std::vector<animation::AnimatedObject> sequences;

        void clear() noexcept {
            skeletalAnimations.clear();
            sequences.clear();
        }

        void clearAndReserve() noexcept {
            {
                auto const size = skeletalAnimations.size();
                skeletalAnimations.clear();
                skeletalAnimations.reserve(size);
            }

            {
                auto const size = sequences.size();
                sequences.clear();
                sequences.reserve(size);
            }
        }

    } _animations;
    
    // render
    rhi::runtime::Rhi * _api = nullptr;
    rhi::runtime::Device * _device = nullptr;

    std::vector<rhi::runtime::PipelinePtr> _pipelines;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

    const graphics::GraphicsService & _graphicsService;
    debug::DebugDraw & _drawHelper;
    std::chrono::time_point<std::chrono::steady_clock> _tryReduceUsedMemoryTime;

    // sprites:
    uint32_t _spritesVertexesCount = 0u;
    uint32_t _lastFramesUsedSpritesVertexCount = 0u;
    std::unique_ptr<graphics::InflightBuffer> _spritesVertices = nullptr;

    uint32_t _spritesIndicesCount = 0u;
    uint32_t _lastFramesUsedSpritesIndicesCount = 0u;
    std::unique_ptr<graphics::InflightBuffer> _spritesIndices = nullptr;
        
    // meshes:
    struct MeshTransformsIndices {
        uint32_t transformIndex = kUnknownTransformIndex;
        uint32_t skeletalTransformIndex = kUnknownTransformIndex;
    };

    std::unordered_map<const AttachmentObject*, MeshTransformsIndices> _meshTransformIndicesMap;
    std::vector<std::pair<const AttachmentObject*, MeshTransformsIndices>> _tmpUniqueMeshes;
    
    uint32_t _meshesDrawInFrame = 0u;
    uint32_t _lastFramesUsedMeshesDraw = 0u;
    std::unique_ptr<graphics::InflightBuffer> _meshTransformIndices = nullptr;

    uint32_t _meshTransformsInFrame = 0u;
    uint32_t _lastFramesUsedMeshsTransforms = 0u;
    std::unique_ptr<graphics::InflightBuffer> _meshTransforms = nullptr;

    uint32_t _skeletalTransformsInFrame = 0u;
    uint32_t _lastFramesUsedSkeletalTransforms = 0u;
    std::unique_ptr<graphics::InflightBuffer> _meshSkeletalTransforms = nullptr;

    // async animation calculation:
    // need use stop sources?
    std::array<threads::WaitSource, 128u> _calculationWaiters;
    std::vector<threads::WaitSource*> _calculationWaiterPtrs;

    // samplers:
    rhi::SamplerValue _linearSampler;
};

template <typename Condition>
void Scene::fillAttachmentsArray(const mat4f & cameraTransform, Node * root, Condition && condition, std::vector<AttachmentObject*> & objects) {
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

        auto * object = n->getAttachment();
        while (object) {
            if (condition(object)) {
                objects.emplace_back(object);

                if constexpr (kDebugDrawBounds) {
                    uint32_t color = 0xff'00'ff'ffu;
                    if (object) {
                        switch (object->typeIndex) {
                        case ctti::typeIndex<AttachmentObject, Skeleton>():
                            color = 0xff'77'22'aau;
                            break;
                        case ctti::typeIndex<AttachmentObject, Mesh>():
                            color = 0xff'22'22'22u;
                            break;
                        case ctti::typeIndex<AttachmentObject, Sprite>():
                            color = 0xff'ff'ff'ffu;
                            break;
                        default:
                            break;
                        }
                    }
                    _drawHelper.drawBounds(bounds, color, cameraTransform * worldTransform);
                }
            }

            object = object->next;
        }

        if constexpr (kDebugDrawGizmos) {
            auto wtr = worldTransform;
            wtr[0u] = glm::normalize(wtr[0u]);
            wtr[1u] = glm::normalize(wtr[1u]);
            wtr[2u] = glm::normalize(wtr[2u]);
            _drawHelper.drawGizmo(0.1f, cameraTransform * wtr);
        }
        return true;
    });

    std::sort(objects.begin(), objects.end(), [](const AttachmentObject * a, const AttachmentObject * b) noexcept {
        return compareAttachmentsForDraw(*a, *b);
    });
}

}
}