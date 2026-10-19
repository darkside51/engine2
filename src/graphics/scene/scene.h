#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/common/draw_helper.h"
#include "math/math.h"
#include "animation.h"
#include "bounds.h"
#include "frustum.h"
#include "node.h"
#include "object.h"
#include "register.h"
#include "types.h"
#include "threads/threads.h"

#include "stl/algorithm.h"
#include "stl/array.h"
#include "stl/span.h"
#include "stl/unordered_map.h"
#include "stl/utility.h"
#include "stl/vector.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>

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
    AnimationInfo animInfo;
    AttachmentObject * object = nullptr;
    float mix = 0.0f;
};
}

// global
void attachObjectToNode(AttachmentObject & object, Node & node) noexcept;

class Scene {
public:
    struct CommonPipeline {
        enum class Type : uint8_t {
            eMeshShadow = 0u,
            eMeshSkinnedShadow = 1u,
            eMeshSkinCompute = 2u,
            eMeshColorId = 3u,
            eMeshSkinColorId = 4u,
            eSpriteColorId = 5u,
            eCount
        };
        stl::array<uint32_t, std::to_underlying(Type::eCount)> pipelines;
    };

    struct DispatchResult {
        uint32_t computeSkinVerticesOffset = 0u;
    };

    Scene(const graphics::GraphicsService & graphicsService, graphics::DrawHelper & drawHelper, const AnimationManager & animationManager);
    ~Scene();

    void destroy() noexcept;

    // nodes
    Node * createNode() noexcept;
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
    // template<typename T>
    // AnimationInfoChain * createAnimInfo(uint16_t animationId, float startTime, float weight = 1.0f) {
    //     return _animationInfosAllocator.create<AnimationInfoChain>(startTime, weight, animationId, ctti::typeIndex<Animation, T>());
    // }
    // bool destroyAnimInfo(AnimationInfoChain * info) noexcept;

    template<typename T>
    AnimationDescription * createAnimInfo(uint16_t animationId, float startTime, float speed = 1.0f, AnimSwitchType switchType = AnimSwitchType::eNone) {
        return _animationsAllocator.create<AnimationDescription>(ctti::typeIndex<Animation, T>(), animationId, startTime, speed, switchType);
    }
    bool destroyAnimInfo(AnimationDescription * info) noexcept;

    // quality levels
    void insertQualityLevels(stl::vector<float> && levels) {
        _qualityLevels.emplace_back(std::move(levels));
    }

    stl::span<const float> getQualityLevels(uint16_t idx) const noexcept;

    // render & compute
    void assignCommonPipeline(uint32_t pipelineId, CommonPipeline::Type type) noexcept;
    uint32_t getCommonPipeline(CommonPipeline::Type type) const noexcept {
        return _commonPipelines.pipelines[std::to_underlying(type)];
    }

    void initRenderData(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
    void beginFrame(std::chrono::time_point<std::chrono::steady_clock> now, float delta) noexcept;
    void endFrame(rhi::runtime::CommandList & commandList, rhi::runtime::CommandList & transferCommandList, uint8_t frame);

    template <typename Condition>
    void fillAttachmentsArray(const ViewParams & viewParams, Node * root, Condition && condition, 
        stl::vector<AttachmentObject*> & objects, FillAttachmentFlags flags);
    DrawDataOffsets prepareDrawData(stl::span<AttachmentObject*> objects, uint8_t frame) noexcept;

    void animate(std::chrono::time_point<std::chrono::steady_clock> now, float delta, stl::span<AttachmentObject*> objects);
    void draw(RenderPass pass, rhi::runtime::CommandList & commandList, uint8_t frame, const ViewParams & viewParams, 
        stl::span<AttachmentObject*> objects, const DrawDataOffsets & offsets, const void * uniformData = nullptr);
    void drawDebug(const ViewParams & viewParams, stl::span<AttachmentObject*> objects);
    DispatchResult dispatch(rhi::runtime::CommandList & commandList, uint8_t frame, stl::span<AttachmentObject*> objects, const DrawDataOffsets & offsets);

    // getters
    bool isComputeSkinningEnabled() const noexcept;

    // attachments
    static void sortObjectsForDraw(stl::vector<AttachmentObject*> & objects) noexcept {
        stl::sort(objects.begin(), objects.end(), [](const AttachmentObject * a, const AttachmentObject * b) noexcept {
            return compareAttachmentsForDraw(*a, *b);
        });
    }

    const graphics::GraphicsService & getGraphicsService() const noexcept {
        return _graphicsService;
    }

    const rhi::SamplerValue & getNearestSampler() const noexcept {
        return _nearestSampler;
    }

    const rhi::SamplerValue & getLinearSampler() const noexcept {
        return _linearSampler;
    }

    float getShadersTime() const noexcept { return _shadersTime; }
    float getAnimationClock() const noexcept { return _animationClock; }
    
    const rhi::runtime::BufferPtr & getViewParamsUniformBuffer(uint8_t frame) const noexcept {
        return (*_viewParamsUniformBuffer)(frame);
    }

    uint32_t getCurrentViewParamsBufferOffset() const noexcept {
        return _currentViewParamsBufferOffset;
    }

    RenderPass getCurrentRenderPass() const noexcept {
        return _currentPass;
    }

    void setAnimationSpeed(float speed) noexcept {
        _animationSpeed = speed;
    }

    float getAnimationSpeed() const noexcept {
        return _animationSpeed;
    }

    // custom delegates
    void addCustomDelegate(ICustomSceneDelegate * delegate) {
        _customDelegates.emplace_back(delegate);
    }

    void removeCustomDelegate(ICustomSceneDelegate * delegate) {
        stl::erase(_customDelegates, delegate); 
    }

    ViewParams buildViewParams(const GraphicsParams & params) const noexcept;

private:
    // attachments
    static bool compareAttachmentsForDraw(const AttachmentObject & a, const AttachmentObject & b) noexcept;
    void calculateQualityFactors(engine::scene::AttachmentObject* object, const ViewParams & viewParams) const noexcept;
    
    uint32_t calculateTargetStagingSize() const noexcept;
    void performCommits(rhi::runtime::CommandList & commandList, uint8_t frame);
    void reduceUsedMemory(std::chrono::time_point<std::chrono::steady_clock> now) noexcept;

    // dispatch
    void dispatchObjects(rhi::runtime::CommandList & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end, uint16_t typeIdx, DrawDataOffsets & offsets);

    void dispatchMeshes(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        DrawDataOffsets & offsets);
    void dispatchSameMeshes(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        uint32_t count, uint32_t transformIndex, uint32_t skeletonsOffset);

    void dispatchCustomObjects(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        DrawDataOffsets & offsets);        

    // draw
    void drawObjects(rhi::runtime::CommandList & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets);

    void drawCommon(rhi::runtime::CommandList & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets);

    void drawShadows(rhi::runtime::CommandList & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets);

    void drawColorIds(rhi::runtime::CommandList & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets);

    void drawDummies(AttachmentsIterator begin, AttachmentsIterator end, const ViewParams & viewParams, uint32_t color);
    void drawSprites(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        const ViewParams & viewParams, uint32_t & vtxOffset, uint32_t & idxOffset);
    void drawSpritesBatch(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const ViewParams & viewParams, uint32_t batchVertexesCount, uint32_t batchIndicesCount, uint32_t vtxOffset, uint32_t idxOffset);

    void drawMeshes(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        const ViewParams & viewParams, uint32_t & offset);
    void drawMeshes2(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        const ViewParams & viewParams, uint32_t & offset, uint32_t & skeletonsOffset);
    void drawMeshes2Shadow(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        const ViewParams & viewParams, uint32_t & offset, uint32_t & skeletonsOffset);
    void drawMeshes2ColorIds(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        const ViewParams & viewParams, uint32_t & offset, uint32_t & skeletonsOffset);
    void drawSameMeshes(rhi::runtime::CommandList & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        uint32_t count, bool skeletal);
    // todo: void drawSameMeshesShadow(...);

    void drawCustomObjects(AttachmentsIterator begin, AttachmentsIterator end, const ViewParams & viewParams, 
        rhi::runtime::CommandList & commandList, uint8_t frame);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false, false>;
    NodesAllocator _nodeAllocator{512u, 256u};

    // using AnimationInfosAllocator = allocators::PoolAllocatorChain<sizeof(AnimationInfoChain), alignof(AnimationInfoChain), false, false>;
    // AnimationInfosAllocator _animationInfosAllocator{512u, 256u};

    using AnimationsAllocator = allocators::PoolAllocatorChain<sizeof(AnimationDescription), alignof(AnimationDescription), false, false>;
    AnimationsAllocator _animationsAllocator{512u, 256u};

    float _animationSpeed = 1.0f;
    float _shadersTime = 0.0f;
    float _animationClock = 0.0f;

    uint32_t _frameNumber = 0u;
    Node * _rootNode = nullptr;

    stl::unordered_map<size_t, std::unique_ptr<ObjectsCollection>> _objectCollections;

    // animation
    // static thread_local stl::vector<animation::AnimatedObject> _sequences; //
    // can use threads::unique_t<animation::AnimatedObject>::id() - if want to create AnimatedObjects on some threads!
    struct AnimationCollection {
        stl::vector<animation::AnimatedObject> skeletalAnimations;
        stl::vector<animation::AnimatedObject> sequences;

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

    void completeAnimations() noexcept;
    stl::vector<stl::vector<float>> _qualityLevels;
    
    // render
    rhi::runtime::Rhi * _api = nullptr;
    rhi::runtime::Device * _device = nullptr;

    stl::vector<rhi::runtime::PipelinePtr> _graphicsPipelines;
    stl::vector<rhi::runtime::PipelinePtr> _computesPipelines;
    std::unique_ptr<graphics::InflightStagingBuffer> _staging = nullptr;

    const graphics::GraphicsService & _graphicsService;
    graphics::DrawHelper & _drawHelper;
    const AnimationManager & _animationManager;
    std::chrono::time_point<std::chrono::steady_clock> _tryReduceUsedMemoryTime;

    uint32_t _currentViewParamsBufferOffset = 0u;
    std::unique_ptr<engine::graphics::InflightBuffer> _viewParamsUniformBuffer = nullptr;

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
        uint32_t skeletonId = kUnknownTransformIndex;
    };

    stl::unordered_map<const AttachmentObject*, MeshTransformsIndices> _meshTransformIndicesMap;
    stl::vector<stl::pair<const AttachmentObject*, MeshTransformsIndices>> _tmpUniqueMeshes;
    
    uint32_t _meshesDrawInFrame = 0u;
    uint32_t _lastFramesUsedMeshesDraw = 0u;
    std::unique_ptr<graphics::InflightBuffer> _meshTransformIndices = nullptr;

    uint32_t _meshTransformsInFrame = 0u;
    uint32_t _lastFramesUsedMeshsTransforms = 0u;
    std::unique_ptr<graphics::InflightBuffer> _meshTransforms = nullptr;

    uint32_t _skeletalTransformsInFrame = 0u;
    uint32_t _lastFramesUsedSkeletalTransforms = 0u;
    std::unique_ptr<graphics::InflightBuffer> _meshSkeletalTransforms = nullptr;

    // compute mesh:
    uint32_t _skeletonsInFrame = 0u;
    uint32_t _uniqueSkeletons = 0u;
    uint32_t _lastFramesUsedSkeletonsInFrame = 0u;
    std::unique_ptr<graphics::InflightBuffer> _skeletonIds = nullptr;

    // outline meshes
    //...

    // color ids meshes
    uint32_t _colorIdSkeletalMeshesIndicesInFrame = 0u;
    uint32_t _lastFramesUsedColorIdSkeletalMeshesIndicesInFrame = 0u;
    std::unique_ptr<graphics::InflightBuffer> _colorIdSkeletalMeshesIds = nullptr;

    uint32_t _colorIdMeshesIndicesInFrame = 0u;
    uint32_t _lastFramesUsedColorIdMeshesIndicesInFrame = 0u;
    std::unique_ptr<graphics::InflightBuffer> _colorIdMeshesIds = nullptr;

    uint32_t _computedVerticesSizeInFrame = 0u; // need resize buffers functional!
    uint32_t _lastFramesComputedVerticesSize = 0u;

    stl::vector<rhi::runtime::BufferPtr> _computedVertices = {};
    stl::vector<rhi::runtime::BufferPtr> _computedNormals = {};
    stl::vector<rhi::runtime::BufferPtr> _computedTangents = {};

    // async animation calculation:
    // need use stop sources?
    stl::array<threads::WaitSource, 128u> _calculationWaiters;
    struct CalculationWaiterPtrs {
        stl::array<threads::WaitSource*, 128u> values;
        uint8_t current = 0u;

        threads::WaitSource * emplace_back(threads::WaitSource * source) noexcept {
            values[current++] = source;
            return source;
        }

        uint8_t size() const noexcept {
            return current;
        }

        void clear() noexcept {
            current = 0u;
        }

        stl::span<threads::WaitSource *> view() noexcept {
            return {values.data(), current};
        }
    } _calculationWaiterPtrs;

    // samplers:
    rhi::SamplerValue _nearestSampler;
    rhi::SamplerValue _linearSampler;

    //
    RenderPass _currentPass = RenderPass::eCommon;
    CommonPipeline _commonPipelines;

    // shadows
    uint8_t _shadowCascades = 0u;
    vec2f _colorPassScreenOffset = {0.0f, 0.0f};

    // custom delegates
    stl::vector<ICustomSceneDelegate *> _customDelegates;
};

template <typename Condition>
void Scene::fillAttachmentsArray(const ViewParams & viewParams, Node * root, Condition && condition, 
    stl::vector<AttachmentObject*> & objects, FillAttachmentFlags flags) {
    const auto currentSize = static_cast<uint32_t>(objects.size()); 
    objects.clear();
    objects.reserve(std::max(kFillNodesReserveSize, currentSize));

    const Frustum frustum(viewParams.cameraTransform);
    recalculateHierarchy(root, [this, &frustum, &viewParams, &objects, &condition, flags](Node * n) noexcept { 
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
            if (!object->isHidden() && condition(object)) {
                objects.emplace_back(object);
                if (static_cast<uint8_t>(flags) & static_cast<uint8_t>(FillAttachmentFlags::eCalculateQuality) > 0u) {
                    calculateQualityFactors(object, viewParams);
                }
            }

            object = object->next;
        }

        if constexpr (kDebugDrawBounds) {
            _drawHelper.drawBounds(bounds, 0xff'00'ff'00u, viewParams.cameraTransform * worldTransform);
        }

        if constexpr (kDebugDrawGizmos) {
            auto wtr = worldTransform;
            wtr[0u] = glm::normalize(wtr[0u]);
            wtr[1u] = glm::normalize(wtr[1u]);
            wtr[2u] = glm::normalize(wtr[2u]);
            _drawHelper.drawGizmo(0.1f, viewParams.cameraTransform * wtr);
        }
        return true;
    });

    stl::sort(objects.begin(), objects.end(), [](const AttachmentObject * a, const AttachmentObject * b) noexcept {
        return compareAttachmentsForDraw(*a, *b);
    });
}

}
}