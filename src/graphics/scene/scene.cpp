#include "scene.h"
#include "animation.h"
#include "animation_manager.h"
#include "animation/texture/frame_sequence_animation.h"
#include "objects/sprite.h"
#include "objects/mesh.h"
#include "objects/custom_object.h"

#include "graphics/common/graphics_service.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/utils/vertex_processing.h"

#include "threads/thread_pool/thread_pool.h"

#include "graphics/common/material.h"
#include "graphics/mesh/model_manager.h"
#include "graphics/texture/texture.h"
#include "graphics/texture/texture_manager.h"

#include "stl/algorithm.h"

#include <cassert>

namespace {
    consteval void _() noexcept {
        using namespace engine;
        using namespace engine::scene;

        // static_assert(sizeof(AnimationInfo) == 12u);
        // static_assert(sizeof(AnimationInfoChain) == 32u);

        static_assert(sizeof(AnimationInfo) == 12u);
        static_assert(sizeof(AnimationDescription) == 32u);

        static_assert(sizeof(AttachmentObject) == 40u);
        static_assert(sizeof(Dummy) == 40u);
        static_assert(sizeof(Mesh) == 200u);
        static_assert(sizeof(Sprite) == 136u);

        #ifdef SIMD_MATH
        static_assert(sizeof(Node) == 288u);
        static_assert(sizeof(Bounds) == 48u);
        static_assert(sizeof(Transformation) == 64u);
        static_assert(alignof(Node) == 32u);
        static_assert(alignof(void*) == 8u);
        static_assert(alignof(mat4f) == 16u);
        static_assert(alignof(Bounds) == 16u);
        static_assert(alignof(Transformation) == 16u);
        #else
        static_assert(sizeof(Node) == 232u);
        static_assert(sizeof(Bounds) == 28u);
        static_assert(sizeof(Transformation) == 40u);
        #endif
        
        static_assert(sizeof(mesh::Attribute) == 8u);
    }
}

namespace engine::scene {
    constexpr float kTryReduceUsedMemoryTimePeriodSeconds = 5.0f;
    constexpr uint32_t kInitialVerticesCountMax = 1024u;
    constexpr uint32_t kInitialIndicesCountMax = 1024u;
    constexpr uint32_t kInitialMeshTransformsInFrameMax = 128u;
    constexpr uint32_t kInitialMeshColorIdsInFrameMax = 128u;
    //constexpr uint32_t kInitialMeshOutlinedInFrameMax = 16u;
    constexpr uint32_t kInitialSkeletalmeshTransformsInFrameMax = 128u;
    constexpr uint32_t kMinStagingSize = 100u * 1024u /*100 Kb*/;
    constexpr uint32_t kStaginBufferEnlargeSizeMult = 2u;
    constexpr uint32_t kViewParamsBufferSizeBytes = 1024u; // 1Kb

    constexpr uint16_t kUndefinedAnimId = 0xff'ffu;
    constexpr float kAnimUpdateFrequency = 1.0f / 120.0f;

    using SsboTransform = mat3x4f; // to use 48bytes transposed matrices
    constexpr uint32_t kSsboTransformSize = sizeof(SsboTransform);
    constexpr uint32_t kDefaultSkeletoneBonesCount = 128u;
    constexpr uint32_t kSsboSkeletalTransformSize = sizeof(SsboTransform) * kDefaultSkeletoneBonesCount;

    constexpr uint8_t kSkeletalAnimationCountToEnableMultyThreading = 32u;
    constexpr uint8_t kAnimationCalculationThreadsCount = 2u;

    // compute skinning
    constexpr bool kComputeSkinningAvailable = true;
    constexpr uint16_t kComputeSkinningLocalGroupSize = 64u;
    constexpr uint32_t kMeshComputeVerticesBufferBytesCount = 128u * 1024u * 1024u; // 128Mb
    constexpr uint32_t kMaxSkeletonsDispatchPack = 2048u;

    // times
    constexpr float kAnimBlendTime = 0.35f; // in seconds
    constexpr float kShaderTimeMultiplier = 1e-3f;

    // global objects:
    FramesSequenceAnimator gSequenceAnimator{1.0f / 30.0f};
    SkeletalAnimator gSkeletalAnimator{};

    threads::ThreadPool gThreadPool{kAnimationCalculationThreadsCount};
    stl::array<SkeletalAnimator, kAnimationCalculationThreadsCount> gSkeletalAnimatorsMT;

    // quality
    constexpr stl::array<float, 7u> kDefaultQualityLevels = {2.0f, 4.0f, 16.0f, 36.0f, 64.0f, 100.0f, 144.0f};

    // struct ColoredVertex {
    //     stl::array<float, 3u> coord;
    //     uint32_t color;
    // };
    
    // struct TexturedVertex {
    //     stl::array<float, 3u> coord;
    //     stl::array<float, 2u> uv;
    // };

    struct SpriteVertex {
        stl::array<float, 3u> position;
        stl::array<uint16_t, 2u> uv;
        stl::array<uint8_t, 4u> color;
    };

    constexpr uint32_t kSpriteVertexSize = sizeof(SpriteVertex);

    void attachObjectToNode(AttachmentObject & object, Node & node) noexcept {
        switch(object.typeIndex) {
        case ctti::typeIndex<AttachmentObject, Skeleton>(): [[fallthrough]];
        case ctti::typeIndex<AttachmentObject, CustomSceneObject>(): [[fallthrough]];
        case ctti::typeIndex<AttachmentObject, Dummy>():
            node.setAttachment(&object);
            return;
        case ctti::typeIndex<AttachmentObject, Mesh>():
            static_cast<Mesh &>(object).attachToNode(&node);
            return;
        case ctti::typeIndex<AttachmentObject, Sprite>():
            static_cast<Sprite &>(object).attachToNode(&node);
            return;
        }
    }

    Scene::Scene(const graphics::GraphicsService & graphicsService, graphics::DrawHelper & drawHelper) : 
        _rootNode(createNode()),
        _graphicsService(graphicsService),
        _drawHelper(drawHelper) {
        // types:
        registerTypes();

        // invalidate _common pipelines:
        memset(_commonPipelines.pipelines.data(), 0xffu, _commonPipelines.pipelines.size() * sizeof(decltype(_commonPipelines.pipelines)::value_type));
    }

    Scene::~Scene() {
        gThreadPool.cancelTasks();
        destroy();

        // for (auto & [_, objects] : _objectCollections) {
        // }
    }

    void Scene::destroy() noexcept {
        destroyNode(_rootNode);
        _rootNode = nullptr;

        _animations.clear();

        _viewParamsUniformBuffer = nullptr;

        _spritesVertices = nullptr;
        _spritesIndices = nullptr;

        _meshTransformIndices = nullptr;
        _meshTransforms = nullptr;
        _meshSkeletalTransforms = nullptr;
        _skeletonIds = nullptr;
        _colorIdMeshesIds = nullptr;
        _colorIdSkeletalMeshesIds = nullptr;

        _computedVertices = {};
        _computedNormals = {};
        _computedTangents = {};
        
        _staging = nullptr;
        _graphicsPipelines.clear();
        _computesPipelines.clear();
    }

    Node * Scene::createNode() noexcept {
        return _nodeAllocator.create<Node>();
    }

    void Scene::destroyNode(Node * node) noexcept {
        if (node == nullptr) {
            return;
        }

        if (node->removeFromParent() || node == _rootNode) {
            if (Node * n = node->firstChild) {
                while (n->next) {
                    auto * n1 = n->next;
                    destroyNode(n);
                    n = n1;
                }
                destroyNode(n);
            }

            if (auto * object = node->getAttachment()) {
                destroyObject(object);
            }

            _nodeAllocator.destroy(node);
        }
    }

    bool Scene::destroyObject(AttachmentObject * object) noexcept {
        const auto id = object->typeIndex;

        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            return false;
        }

        if (object->animInfo) {
            destroyAnimInfo(object->animInfo);
            object->animInfo = nullptr;
        }

        bool result = true;
        if (object->next) {
            result = result && destroyObject(object->next);
        }

        result = result && it->second->destroyObject(object);
        return result;
    }

    bool Scene::destroyAnimInfo(AnimationDescription * info) noexcept {
        if (const bool result = (info != nullptr)) {
            _animationsAllocator.destroy(info);
            return true;
        }
        return false;
        // while (info) {
        //     auto next = info->next;
        //     _animationInfosAllocator.destroy(info);
        //     info = next;
        // }
        // return result;
    }

    uint32_t Scene::registerGraphicsPipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) {
            _graphicsPipelines.push_back(std::move(pipeline));
            return _graphicsPipelines.size() - 1u;
        }

        return 0xff'ff'ff'ffu;
    }

    const rhi::runtime::PipelinePtr & Scene::getGraphicsPipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _graphicsPipelines.size()) {
            static constexpr rhi::runtime::PipelinePtr kNullPipeline = nullptr;
            return kNullPipeline;
        }
        return _graphicsPipelines[pipelineId];
    }

    void Scene::assignCommonPipeline(uint32_t pipelineId, CommonPipeline::Type type) noexcept {
        _commonPipelines.pipelines[std::to_underlying(type)] = pipelineId;
    }

    uint32_t Scene::registerComputePipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) {
            _computesPipelines.push_back(std::move(pipeline));
            return _computesPipelines.size() - 1u;
        }

        return 0xff'ff'ff'ffu;
    }

    const rhi::runtime::PipelinePtr & Scene::getComputePipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _computesPipelines.size()) {
            static constexpr rhi::runtime::PipelinePtr kNullPipeline = nullptr;
            return kNullPipeline;
        }
        return _computesPipelines[pipelineId];
    }

    bool Scene::isComputeSkinningEnabled() const noexcept {
        return kComputeSkinningAvailable;
    }

    void Scene::initRenderData(rhi::runtime::Rhi & api, rhi::runtime::Device & device) {
        _api = &api;
        _device = &device;

        // samplers
        _linearSampler.info.magFilter = rhi::FilterMode::eLinear;
        _linearSampler.info.minFilter = rhi::FilterMode::eLinear;
        _linearSampler.info.mipMapFilter = rhi::FilterMode::eLinear;
        // _linearSampler.info.addressModeU = rhi::SamplerAddressMode::eClamp_To_Edge;
        // _linearSampler.info.addressModeV = rhi::SamplerAddressMode::eClamp_To_Edge;
        // _linearSampler.info.addressModeW = rhi::SamplerAddressMode::eClamp_To_Edge;
        _linearSampler.info.addressModeU = rhi::SamplerAddressMode::eRepeat;
        _linearSampler.info.addressModeV = rhi::SamplerAddressMode::eRepeat;
        _linearSampler.info.addressModeW = rhi::SamplerAddressMode::eRepeat;
        
        _nearestSampler.info.magFilter = rhi::FilterMode::eNearest;
        _nearestSampler.info.minFilter = rhi::FilterMode::eNearest;
        _nearestSampler.info.mipMapFilter = rhi::FilterMode::eNearest;
        _nearestSampler.info.addressModeU = rhi::SamplerAddressMode::eRepeat;
        _nearestSampler.info.addressModeV = rhi::SamplerAddressMode::eRepeat;
        _nearestSampler.info.addressModeW = rhi::SamplerAddressMode::eRepeat;

        device.evaluateSampler(_nearestSampler);
        device.evaluateSampler(_linearSampler);

        // viewParams:
        const rhi::BufferInfo unifromsInfo {
            rhi::BufferUsageFlags::eUniform_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal | rhi::MemoryPropertyFlags::eHostVisible
        };
        const size_t uniformBufferOffsetAlign = _device->getUniformBufferOffsetAlignment();

        _viewParamsUniformBuffer = std::make_unique<graphics::InflightBuffer>(
            api, device, unifromsInfo, kViewParamsBufferSizeBytes
        );
        _viewParamsUniformBuffer->createBuffers();
        
        //create vertex and index buffers
        const rhi::BufferInfo verticesInfo {
            rhi::BufferUsageFlags::eVertex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        const rhi::BufferInfo indicesInfo {
            rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        const rhi::BufferInfo verticesInfo2 {
            rhi::BufferUsageFlags::eVertex_Buffer /*| rhi::BufferUsageFlags::eUniform_Buffer*/ | 
            rhi::BufferUsageFlags::eStorage_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        // sprites:
        // vertices
        _spritesVertices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, kSpriteVertexSize * kInitialVerticesCountMax
        );

        // indices
        _spritesIndices = std::make_unique<graphics::InflightBuffer>(
            api, device, indicesInfo, sizeof(uint32_t) * kInitialIndicesCountMax
        );

        // meshes:
        const rhi::BufferInfo storageInfo {
            rhi::BufferUsageFlags::eStorage_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        _meshTransforms = std::make_unique<graphics::InflightBuffer>(
            api, device, storageInfo, kSsboTransformSize * kInitialMeshTransformsInFrameMax
        );

        _meshSkeletalTransforms = std::make_unique<graphics::InflightBuffer>(
            api, device, storageInfo, kSsboSkeletalTransformSize * kInitialSkeletalmeshTransformsInFrameMax
        );

        _meshTransformIndices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo2, sizeof(uint32_t) * kInitialMeshTransformsInFrameMax
        );

        _skeletonIds = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo2, sizeof(uint32_t) * kInitialMeshTransformsInFrameMax
        );

        _colorIdMeshesIds = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo2, sizeof(vec2u) * kInitialMeshColorIdsInFrameMax
        );

        _colorIdSkeletalMeshesIds = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo2, sizeof(vec2u) * kInitialMeshColorIdsInFrameMax
        );

        if constexpr (kComputeSkinningAvailable) {
            // compute:
            const rhi::BufferInfo computeVerticesInfo { 
                rhi::BufferUsageFlags::eStorage_Texel_Buffer,
                rhi::MemoryPropertyFlags::eDeviceLocal
            };

            // reserve 2u * kMeshComputeVerticesBufferBytesCount mb gpu memory for calculate skinnig all vertices of all meshes 
            // maybe calculate and change this size dynamically is better way - 100%

            const auto buffersCount = device.inFlightFramesCount();
            _computedVertices.reserve(buffersCount);
            _computedNormals.reserve(buffersCount);
            _computedTangents.reserve(buffersCount);

            // _computedVertices    - data vec4 - r32g32b32a32 as texelbuffer
            // _computedNormals     - data vec4 - r8g8b8a8 as texelbuffer
            // _computedTangents    - data vec4 - r8g8b8a8 as texelbuffer

            for (uint8_t i = 0u; i < buffersCount; ++i) {
                _computedVertices.emplace_back(api.createBuffer(device, computeVerticesInfo, kMeshComputeVerticesBufferBytesCount));
                _computedNormals.emplace_back(api.createBuffer(device, computeVerticesInfo, kMeshComputeVerticesBufferBytesCount / 4u));
                _computedTangents.emplace_back(api.createBuffer(device, computeVerticesInfo, kMeshComputeVerticesBufferBytesCount / 4u));
            }
        }
    }

    bool Scene::compareAttachmentsForDraw(const AttachmentObject & a, const AttachmentObject & b) noexcept {
        // if (a.layer != b.layer) {
        //     return a.layer < b.layer;
        // }

        if (a.order != b.order) {
            return a.order < b.order;
        }

        if (a.typeIndex != b.typeIndex) {
            return a.typeIndex < b.typeIndex;
        }

        switch(a.typeIndex) {
        case ctti::typeIndex<AttachmentObject, Skeleton>():
            return static_cast<const Skeleton &>(a).cmp(static_cast<const Skeleton &>(b));
        case ctti::typeIndex<AttachmentObject, Mesh>():
            return static_cast<const Mesh &>(a).cmp(static_cast<const Mesh &>(b));
        case ctti::typeIndex<AttachmentObject, Sprite>():
            return static_cast<const Sprite &>(a).cmp(static_cast<const Sprite &>(b));
        case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
            return static_cast<const CustomSceneObject &>(a).cmp(static_cast<const CustomSceneObject &>(b));
        case ctti::typeIndex<AttachmentObject, Dummy>():
            return static_cast<const Dummy &>(a).cmp(static_cast<const Dummy &>(b));
        }

        return false;
    }

    stl::span<const float> Scene::getQualityLevels(uint16_t idx) const noexcept {
        if (idx < _qualityLevels.size()) {
            return _qualityLevels[idx];
        }

        return kDefaultQualityLevels;
    }

    void Scene::calculateQualityFactors(engine::scene::AttachmentObject* object, const ViewParams & viewParams) const noexcept {
        const auto position = engine::vec3f{object->node->getWorldTransform()[3u]}; 
        const auto length2 = glm::length2(position - viewParams.cameraPosition);
        object->animationQualityFactor = 0u;
        auto const levels = getQualityLevels(object->qualityLevelsId);
        for (const auto l2 : levels) {
            if (l2 > length2) {
                break;
            }
            ++object->animationQualityFactor;
        }
    }

    void Scene::animate(std::chrono::time_point<std::chrono::steady_clock> now, float delta, stl::span<AttachmentObject*> objects, 
        const AnimationManager & animationManager) {
        //return;
        if (objects.empty()) {
            return;
        }

        // static thread_local stl::vector<animation::AnimatedObject> sequences; // todo: remove static
        
        const auto skeletalAnimationsCountOnStart = _animations.skeletalAnimations.size();
        const auto sequenceAnimationsCountOnStart = _animations.sequences.size();
        const float mixDelta = delta * std::abs(_animationSpeed) / kAnimBlendTime;

        for (auto * object : objects) {
            auto * animInfo = object->animInfo;
            if (animInfo) {
                const uint32_t executionFramesCharacter = std::max(
                    static_cast<uint32_t>(std::roundf(kAnimUpdateFrequency * (1u + object->animationQualityFactor) / delta)), 
                    1u);
                const bool isTargetFrame = ((object->id + _frameNumber) % executionFramesCharacter) == 0u;

                if (isTargetFrame) {
                    switch (animInfo->current.animationType) {
                    case ctti::typeIndex<Animation, SkeletalAnimation>() : 
                    {
                        _animations.skeletalAnimations.emplace_back(animInfo->current, object);
                        if (animInfo->target.animationType != kUndefinedAnim) {
                            _animations.skeletalAnimations.emplace_back(animInfo->target, object, animInfo->mix);

                            if (animInfo->animSwitchType == AnimSwitchType::eByTime) {
                                animInfo->mix += mixDelta * animInfo->target.speed;
                                if (animInfo->mix >= 1.0f) {
                                    animInfo->mix = 0.0f;
                                    animInfo->current = animInfo->target;
                                    animInfo->target.animationType = kUndefinedAnim;
                                }
                            }
                        }
                    }
                        break;
                    case ctti::typeIndex<Animation, FramesSequence>() : 
                    {
                        _animations.sequences.emplace_back(animInfo->current, object);
                    }
                        break;
                    default:
                        break;
                    }
                } 

                //animInfo = animInfo->next;
            }
        }

        const std::chrono::duration<float> nowDuration = now.time_since_epoch();
        const float nowTimeCount = nowDuration.count() * _animationSpeed;

        if (!_animations.skeletalAnimations.empty()) { // skeletal animations
            auto begin = _animations.skeletalAnimations.begin() + skeletalAnimationsCountOnStart;
            // stl::sort(begin, _animations.skeletalAnimations.end(), [](const auto & animObject0, const auto & animObject1) {
            //             if (animObject0.object != animObject1.object) {
            //                 return std::less<AttachmentObject*>{}(animObject0.object, animObject1.object);
            //             }
            //             return animObject0.animInfo->animationId < animObject1.animInfo->animationId;
            //         });

            // need sort?
            stl::stable_sort(begin, _animations.skeletalAnimations.end(), [](const auto & animObject0, const auto & animObject1) {
                        return std::less<AttachmentObject*>{}(animObject0.object, animObject1.object);
                    });

            auto animationsCount = static_cast<uint32_t>(_animations.skeletalAnimations.size() - skeletalAnimationsCountOnStart);

            if (animationsCount > kSkeletalAnimationCountToEnableMultyThreading) {
                const auto numWorkers = static_cast<uint32_t>(gThreadPool.numWorkers());
                const auto taskPerThread = std::max(animationsCount / numWorkers, 1u);
                
                auto it = begin;
                size_t i = 0u;
                while (animationsCount > 0u) {
                    /*const*/ auto count = (i == numWorkers - 1u) ? animationsCount : taskPerThread;

                    // get all object animations for last pack object to single thread
                    while (count < animationsCount && ((it + count - 1)->object == (it + count)->object)) {
                        ++count;
                    }
                    const auto animations = stl::span{it, it + count};
                    it += count;
                    
                    const auto w8Id = _calculationWaiterPtrs.size();
                    auto * taskW8 = _calculationWaiterPtrs.emplace_back(&_calculationWaiters[w8Id]);

                    gThreadPool.execute([animator = &gSkeletalAnimatorsMT[i], &animationManager, nowTimeCount, animations, taskW8]() {
                        auto animId = kUndefinedAnimId;
                        const SkeletalAnimation * currentAnim = nullptr;
                        AttachmentObject * currentObject = nullptr;

                        for (const auto & [animInfo, object, mix] : animations) {
                            if (animInfo.animationId != animId) {
                                animId = animInfo.animationId;
                                currentAnim = animationManager.getAnimation<SkeletalAnimation>(animId);
                            }

                            if (currentAnim) {
                                auto * skeleton = static_cast<Skeleton*>(object);
                                animator->enlarge2(skeleton->nodesCount());
                                animator->update2(nowTimeCount * animInfo.speed - animInfo.startTime, *currentAnim);

                                if (currentObject == object) {
                                    animator->apply2(*skeleton, mix);
                                } else {
                                    animator->apply2(*skeleton);
                                }
                            }

                            currentObject = object;
                        }

                        taskW8->request_complete();
                    });

                    animationsCount -= count;
                    ++i;
                }
            } else {
                auto animId = kUndefinedAnimId;
                AttachmentObject * currentObject = nullptr;
                const SkeletalAnimation * currentAnim = nullptr;

                for (const auto & [animInfo, object, mix] : _animations.skeletalAnimations) {
                    if (animInfo.animationId != animId) {
                        animId = animInfo.animationId;
                        currentAnim = animationManager.getAnimation<SkeletalAnimation>(animId);
                    }

                    if (currentAnim) {
                        auto * skeleton = static_cast<Skeleton*>(object);
                        gSkeletalAnimator.enlarge2(skeleton->nodesCount());
                        gSkeletalAnimator.update2(nowTimeCount * animInfo.speed - animInfo.startTime, *currentAnim);

                        if (currentObject == object) {
                            gSkeletalAnimator.apply2(*skeleton, mix);
                        } else {
                            gSkeletalAnimator.apply2(*skeleton);
                        }
                    }

                    currentObject = object;
                }
            }
        }

        if (!_animations.sequences.empty()) { // sequences
            auto begin = _animations.sequences.begin() + sequenceAnimationsCountOnStart;
            stl::sort(begin, _animations.sequences.end(), [](const auto & seq0, const auto & seq1) {
                        return seq0.animInfo.animationId < seq1.animInfo.animationId;
                    });

            auto animId = kUndefinedAnimId;
            const FramesSequence * currentAnim = nullptr;
            
            const auto animations = stl::span{begin, _animations.sequences.end()};
            for (const auto & [animInfo, object, _] : animations) {
                if (animInfo.animationId != animId) {
                    animId = animInfo.animationId;
                    currentAnim = animationManager.getAnimation<FramesSequence>(animId);
                }

                if (currentAnim) {
                    static_cast<Sprite*>(object)->setFrame(gSequenceAnimator.update2(nowTimeCount - animInfo.startTime, *currentAnim));
                }
            }
        }
    }

    DrawDataOffsets Scene::prepareDrawData(stl::span<AttachmentObject*> objects, uint8_t frame) noexcept {
        // resize buffers
        const auto spritesVertexesCount = _spritesVertexesCount;
        const auto spritesIndicesCount = _spritesIndicesCount;

        const auto meshesDrawInFrame = _meshesDrawInFrame;
        auto meshTransformsInFrame = _meshTransformsInFrame;
        const auto skeletonsInFrame = _skeletonsInFrame;
        const auto colorIdMeshesIds = _colorIdMeshesIndicesInFrame;
        const auto colorIdSkeletalMeshesIds = _colorIdSkeletalMeshesIndicesInFrame;

        auto skeletalTransformsInFrame = _skeletalTransformsInFrame;
        _tmpUniqueMeshes.reserve(kInitialMeshTransformsInFrameMax);

        auto addSkeletonToMap = [&](const Skeleton* skeleton)-> stl::pair<uint32_t, uint32_t> {
            const auto result =  stl::pair{skeletalTransformsInFrame, _uniqueSkeletons};
            const auto [it, success] = _meshTransformIndicesMap.emplace(skeleton, 
                        MeshTransformsIndices{
                            .transformIndex = kUnknownTransformIndex,
                            .skeletalTransformIndex = skeletalTransformsInFrame,
                            .skeletonId = _uniqueSkeletons
                        });
            assert(success);
            skeletalTransformsInFrame += skeleton->skinMatricesCount();
            ++_uniqueSkeletons;
            return result;
        };

        for (auto const * object : objects) {
            switch (object->typeIndex) {
            case ctti::typeIndex<AttachmentObject, Skeleton>():
            {
                if (!_meshTransformIndicesMap.contains(object)) { 
                    addSkeletonToMap(static_cast<const Skeleton*>(object));
                }
            }
                break;
            case ctti::typeIndex<AttachmentObject, Mesh>():
            {
                if (!_meshTransformIndicesMap.contains(object)) {
                    const auto * mesh = static_cast<const Mesh*>(object);
                    const auto * skeleton = mesh->getSkeleton();
                    const bool isSkeletalMesh = skeleton != nullptr;

                    uint32_t skeletalTransformIndex = kUnknownTransformIndex;
                    uint32_t skeletonId = kUnknownTransformIndex;
                    const bool colorIdEnabled = mesh->isEnableColorId();

                    if (isSkeletalMesh) { 
                        if (auto it = _meshTransformIndicesMap.find(skeleton); it != _meshTransformIndicesMap.end()) {
                            skeletalTransformIndex = it->second.skeletalTransformIndex;
                            skeletonId = it->second.skeletonId;
                        } else {
                            auto [transfromIndex, skeletonIndex] = addSkeletonToMap(skeleton);
                            skeletalTransformIndex = transfromIndex;
                            skeletonId = skeletonIndex;
                        }

                        if (colorIdEnabled) {
                            ++_colorIdSkeletalMeshesIndicesInFrame;
                        }
                    } else {
                        if (colorIdEnabled) {
                            ++_colorIdMeshesIndicesInFrame;
                        }
                    }
                    
                    const auto [it, success] = _meshTransformIndicesMap.emplace(object, 
                        MeshTransformsIndices{
                            .transformIndex = static_cast<uint32_t>(isSkeletalMesh ? kUnknownTransformIndex : meshTransformsInFrame),
                            .skeletalTransformIndex = skeletalTransformIndex,
                            .skeletonId = skeletonId
                        });
                    assert(success);

                    _tmpUniqueMeshes.emplace_back(it->first, it->second);
                
                    if (!isSkeletalMesh) {
                        ++meshTransformsInFrame;
                    }
                }
                if (static_cast<const Mesh*>(object)->getSkeleton()) {
                    ++_skeletonsInFrame;
                }
                ++_meshesDrawInFrame; // every draw mesh has offset (for get correct matrices from meshTransforms)
            }
                break;
            case ctti::typeIndex<AttachmentObject, Sprite>():
                _spritesVertexesCount += 4u;
                _spritesIndicesCount += 6u;
                break;
            case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
                break;
            case ctti::typeIndex<AttachmentObject, Dummy>():
                break;
            }
        }

        {
            const auto size = static_cast<uint32_t>(_spritesVertexesCount * kSpriteVertexSize);
            const auto currentSize = _spritesVertices->sizeInBytes();
            if (currentSize <= size) {
                _spritesVertices->resize(std::max(currentSize * 2u, size));
            }
        }

        {
            const auto size = static_cast<uint32_t>(_spritesIndicesCount * sizeof(uint32_t));
            const auto currentSize = _spritesIndices->sizeInBytes();
            if (currentSize <= size) {
                _spritesIndices->resize(std::max(currentSize * 2u, size));
            }
        }

        {
            const auto size = static_cast<uint32_t>(meshTransformsInFrame * kSsboTransformSize);
            const auto currentSize = _meshTransforms->sizeInBytes();
            if (currentSize <= size) {
                _meshTransforms->resize(std::max(currentSize * 2u, size));
            }
        }

        {
            const auto size = static_cast<uint32_t>(skeletalTransformsInFrame * kSsboTransformSize);
            const auto currentSize = _meshSkeletalTransforms->sizeInBytes();
            if (currentSize <= size) {
                _meshSkeletalTransforms->resize(std::max(currentSize * 2u, size));
            }
        }

        {
            const auto size = static_cast<uint32_t>(_meshesDrawInFrame * sizeof(uint32_t));
            const auto currentSize = _meshTransformIndices->sizeInBytes();
            if (currentSize <= size) {
                _meshTransformIndices->resize(std::max(currentSize * 2u, size));
            }

            const auto size2 = static_cast<uint32_t>(_skeletonsInFrame * sizeof(uint32_t));
            const auto currentSize2 = _skeletonIds->sizeInBytes();
            if (currentSize2 <= size2) {
                _skeletonIds->resize(std::max(currentSize2 * 2u, size2));
            }

            const auto size3 = static_cast<uint32_t>(_colorIdMeshesIndicesInFrame * sizeof(vec2u));
            const auto currentSize3 = _colorIdMeshesIds->sizeInBytes();
            if (currentSize3 <= size3) {
                _colorIdMeshesIds->resize(std::max(currentSize3 * 2u, size3));
            }

            const auto size4 = static_cast<uint32_t>(_colorIdSkeletalMeshesIndicesInFrame * sizeof(vec2u));
            const auto currentSize4 = _colorIdSkeletalMeshesIds->sizeInBytes();
            if (currentSize4 <= size4) {
                _colorIdSkeletalMeshesIds->resize(std::max(currentSize4 * 2u, size4));
            }
        }

        _spritesVertices->applySize(frame);
        _spritesIndices->applySize(frame);
        _meshTransforms->applySize(frame);
        _meshSkeletalTransforms->applySize(frame);
        _meshTransformIndices->applySize(frame);
        _skeletonIds->applySize(frame);
        _colorIdMeshesIds->applySize(frame);
        _colorIdSkeletalMeshesIds->applySize(frame);

        // staging buffer
        if (auto const stagingTargetSize = calculateTargetStagingSize(); stagingTargetSize > 0u) {
            if (_staging == nullptr || stagingTargetSize >= _staging->sizeInBytes()) { // enlarge staging
                _staging = std::make_unique<graphics::InflightStagingBuffer>(*_api, *_device, 
                    std::max(stagingTargetSize * kStaginBufferEnlargeSizeMult, kMinStagingSize), 
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
            }
        }

        if (_staging) {
            _staging->map(frame); // reset staging data into + mapped memory set frame

            _spritesVertices->setStagingBuffer(_staging.get());
            _spritesIndices->setStagingBuffer(_staging.get());
            _meshTransforms->setStagingBuffer(_staging.get());
            _meshSkeletalTransforms->setStagingBuffer(_staging.get());
            _meshTransformIndices->setStagingBuffer(_staging.get());
            _skeletonIds->setStagingBuffer(_staging.get());
            _colorIdMeshesIds->setStagingBuffer(_staging.get());
            _colorIdSkeletalMeshesIds->setStagingBuffer(_staging.get());
        }

        if (!_tmpUniqueMeshes.empty()) { // mehses transforms to storage buffers
            auto * mem = _meshTransforms->memory2<SsboTransform>();
            auto * mem2 = _meshSkeletalTransforms->memory2<SsboTransform>();
            for (const auto & [object, idx] : _tmpUniqueMeshes) {
                if (idx.transformIndex != kUnknownTransformIndex) {
                    memcpy(
                        (mem + idx.transformIndex), 
                        &glm::transpose(object->node->getWorldTransform())[0u][0u], 
                        sizeof(SsboTransform)
                    );

                    // const auto tr = glm::transpose(object->node->getWorldTransform());
                    // *(mem + idx.transformIndex) = {
                    //     tr[0u],
                    //     tr[1u],
                    //     tr[2u]
                    // };
                    // *(mem + idx) = object->node->getWorldTransform();
                } else if (idx.skeletalTransformIndex != kUnknownTransformIndex) {
                    const auto * skeleton = static_cast<const Mesh*>(object)->getSkeleton();
                    assert(skeleton != nullptr);
                    skeleton->fillSkinMatrices2({mem2 + idx.skeletalTransformIndex, skeleton->skinMatricesCount()});
                }
            }

            if (meshTransformsInFrame > _meshTransformsInFrame) {
                _meshTransforms->modify(meshTransformsInFrame * kSsboTransformSize);
                _meshTransformsInFrame = meshTransformsInFrame;
            }

            if (skeletalTransformsInFrame > _skeletalTransformsInFrame) {
                _meshSkeletalTransforms->modify(skeletalTransformsInFrame * kSsboTransformSize);
                _skeletalTransformsInFrame = skeletalTransformsInFrame;
            }

            _tmpUniqueMeshes.clear();
        }

        return {
            .meshesDrawOffset = meshesDrawInFrame,
            .spriteVerticesOffset = spritesVertexesCount,
            .spriteIndicesOffset = spritesIndicesCount,
            .skeletonOffset = skeletonsInFrame,
            .computeSkinVerticesOffset = 0u,
            .colorIdMeshesOffset = colorIdMeshesIds,
            .colorIdSkeletalMeshesOffset = colorIdSkeletalMeshesIds
        };
    }

    void Scene::dispatchObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets) {

        switch (typeIdx) {
        case ctti::typeIndex<AttachmentObject, Skeleton>():
            break;
        case ctti::typeIndex<AttachmentObject, Mesh>():
        {
            // uint32_t count = end - begin;
            // while (count > 0u) {
            //     const auto c = std::min(kMaxSkeletonsDispatchPack, count);
            //     dispatchMeshes(commandList, frame, begin, begin + c, offsets);
            //     begin += c;
            //     count -= c;
            // }

            dispatchMeshes(commandList, frame, begin, end, offsets);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Sprite>():
            break;
        case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
        {
            dispatchCustomObjects(commandList, frame, begin, end, offsets);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Dummy>():
            break;
        }
    }

    void Scene::dispatchCustomObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        DrawDataOffsets & offsets) {

        auto currentIt = begin;
        auto & object0 = *static_cast<CustomSceneObject*>(*currentIt);
        auto * delegate = &object0.getDelegate();

        for (auto it = begin + 1; it != end; ++it) {
            auto & object = *static_cast<CustomSceneObject*>(*it);
            auto * delegate2 = &object.getDelegate();

            if (delegate != delegate2) {
                if (delegate->isDispatchEnabled()) {
                    delegate->dispatch(*this, commandList, frame, currentIt, it);
                }
                delegate = delegate2;
                currentIt = it;
            }
        }

        if (delegate->isDispatchEnabled()) {
            delegate->dispatch(*this, commandList, frame, currentIt, end);
        }
    }

    void Scene::dispatchMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        DrawDataOffsets & offsets) {
            
        auto currentIt = begin;
        auto it = currentIt;
        const auto & mesh = *static_cast<const Mesh*>(*it);
        auto geometryDataIndex = mesh.geometryDataIndex();
        bool skeletal = mesh.getSkeleton() != nullptr;
        auto dataIndex = mesh.dataIndex();
        auto materialIndex = mesh.materialIndex();

        stl::array<uint32_t, kMaxSkeletonsDispatchPack> skeletonMeshesVerticesCounts{};

        const auto startOffset = offsets.meshesDrawOffset;
        const auto startSkeletonOffset = offsets.skeletonOffset;
        const auto startSkeletalMeshColorIdsOffset = offsets.colorIdSkeletalMeshesOffset;
        const auto startMeshColorIdsOffset = offsets.colorIdMeshesOffset;
        uint32_t i = 0u;
        uint32_t j = 0u;
        uint32_t n = 0u; // for skeletal meshes
        uint32_t n1 = 0u; // for no skeletal meshes
        auto * memory = _meshTransformIndices->memory2<uint32_t>() + startOffset;
        auto * memory2 = _skeletonIds->memory2<uint32_t>() + startSkeletonOffset;
        auto * memory3 = _colorIdMeshesIds->memory2<vec2u>() + startMeshColorIdsOffset;
        auto * memory4 = _colorIdSkeletalMeshesIds->memory2<vec2u>() + startSkeletalMeshColorIdsOffset;
        const auto & ti = _meshTransformIndicesMap.find(*it)->second;

        uint32_t minId = 0xff'ff'ff'ffu;
        uint32_t maxId = 0u;

        uint32_t colorId = mesh.node->getId();
        bool colorIdEnabled = mesh.isEnableColorId();

        if (skeletal) {
            // auto & skeletonData = _meshTransformIndicesMap[*it].skeletonId;
            // const auto vertexCount = mesh.geometry().uniqueIndicesCount + (skeletonData >> 16u) & 0xffffu;
            // skeletonData = (skeletonData & 0xffffu) | (vertexCount << 16u);
            const auto skeletonId = ti.skeletonId;
            assert(skeletonId < kMaxSkeletonsDispatchPack);

            skeletonMeshesVerticesCounts[skeletonId] = mesh.getVertexCount();
            memory[i] = ti.skeletalTransformIndex;
            memory2[j] = skeletonId;// | (vertexCount << 16u);
            minId = std::min(minId, skeletonId);
            maxId = std::max(maxId, skeletonId);

            if (colorIdEnabled) {
                memory4[n] = {skeletonId, colorId};
                ++n;
                //++n1;
            }

            ++j;
        } else {
            memory[i] = ti.transformIndex;

            if (colorIdEnabled) {
                memory3[n1] = {ti.transformIndex, colorId};
                ++n1;
            }
        }

        ++it;
        ++i;

        uint32_t count = 1u;

        const auto & pipeline = getComputePipeline(getCommonPipeline(CommonPipeline::Type::eMeshSkinCompute));
        assert(pipeline != nullptr);

        commandList->bindPipeline(pipeline);
        commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 0u, 0u);
        commandList->bindStorageTexelBuffer(1u, _computedNormals[frame], rhi::Format::eR8G8B8A8_Snorm, 0u, 0u);
        commandList->bindStorageTexelBuffer(2u, _computedTangents[frame], rhi::Format::eR8G8B8A8_Snorm, 0u, 0u);

        commandList->bindStorageBuffer(0u, (*_meshSkeletalTransforms)(frame), 1u, 0u);
        commandList->bindStorageBuffer(1u, (*_meshTransformIndices)(frame), 1u, 0u);
        commandList->bindStorageBuffer(2u, (*_skeletonIds)(frame), 1u, 0u);

        auto computesDataIndex = dataIndex;
        if (skeletal) {
            commandList->bindStorageBuffer(0u, mesh.getUniqueIndices().buffer, 2u, 0u);
            commandList->bindTexelBuffer(1u, mesh.getAttribute(mesh::Semantic::ePosition).buffer, rhi::Format::eR16G16B16A16_Sfloat, 2u);
            commandList->bindTexelBuffer(2u, mesh.getAttribute(mesh::Semantic::eNormal).buffer, rhi::Format::eR8G8B8A8_Snorm, 2u);
            commandList->bindTexelBuffer(3u, mesh.getAttribute(mesh::Semantic::eJoints).buffer, rhi::Format::eR8G8B8A8_Unorm, 2u);
            commandList->bindTexelBuffer(4u, mesh.getAttribute(mesh::Semantic::eWeights).buffer, rhi::Format::eR16G16B16A16_Sfloat, 2u);
            commandList->bindTexelBuffer(5u, mesh.getAttribute(mesh::Semantic::eTangent).buffer, rhi::Format::eR8G8B8A8_Snorm, 2u);
        }

        for (; it != end; ++it, ++i) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            const auto currentGeometryDataIndex = mesh2.geometryDataIndex();
            const bool currentSkeletal = mesh2.getSkeleton() != nullptr;
            const auto currenDataIndex = mesh2.dataIndex();
            const auto currenMaterialIndex = mesh2.materialIndex();
            const auto currentColorIdEnabled = mesh2.isEnableColorId();
            colorId = mesh2.node->getId();

            const auto & ti = _meshTransformIndicesMap.find(*it)->second;
            if (currentSkeletal) {
                // auto & skeletonData = _meshTransformIndicesMap[*it].skeletonId;
                // const auto vertexCount = mesh2.geometry().uniqueIndicesCount + (skeletonData >> 16u) & 0xffffu;
                // skeletonData = (skeletonData & 0xffffu) | (vertexCount << 16u);
                const auto skeletonId = ti.skeletonId;
                assert(skeletonId < kMaxSkeletonsDispatchPack);

                skeletonMeshesVerticesCounts[skeletonId] = mesh2.getVertexCount();
                memory[i] = ti.skeletalTransformIndex;
                memory2[j] = skeletonId;// | (vertexCount << 16u);
                minId = std::min(minId, skeletonId);
                maxId = std::max(maxId, skeletonId);

                if (currentColorIdEnabled) {
                    memory4[n] = {skeletonId, colorId};
                    ++n;
                    //++n1;
                }

                ++j;
            } else {
                memory[i] = ti.transformIndex;

                if (currentColorIdEnabled) {
                    memory3[n1] = {ti.transformIndex, colorId};
                    ++n1;
                }
            }

            const bool completePack = currentSkeletal != skeletal ||
                                currenDataIndex != dataIndex ||
                                currentGeometryDataIndex != geometryDataIndex;
                 
            if (completePack) {
                if (skeletal) { // dispatch
                    dispatchSameMeshes(commandList, frame, currentIt, it, count, offsets.meshesDrawOffset, offsets.skeletonOffset);
                    offsets.skeletonOffset += count;
                }
                offsets.meshesDrawOffset += count;

                colorIdEnabled = currentColorIdEnabled;

                // begin next pack
                dataIndex = currenDataIndex;
                geometryDataIndex = currentGeometryDataIndex;
                skeletal = currentSkeletal;
                materialIndex = currenMaterialIndex;
                currentIt = it;
                count = 1u;

                if (skeletal && computesDataIndex != dataIndex) {
                    computesDataIndex = dataIndex;
                    commandList->bindStorageBuffer(0u, mesh2.getUniqueIndices().buffer, 2u, 0u);
                    commandList->bindTexelBuffer(1u, mesh2.getAttribute(mesh::Semantic::ePosition).buffer, rhi::Format::eR16G16B16A16_Sfloat, 2u);
                    commandList->bindTexelBuffer(2u, mesh2.getAttribute(mesh::Semantic::eNormal).buffer, rhi::Format::eR8G8B8A8_Snorm, 2u);
                    commandList->bindTexelBuffer(3u, mesh2.getAttribute(mesh::Semantic::eJoints).buffer, rhi::Format::eR8G8B8A8_Unorm, 2u);
                    commandList->bindTexelBuffer(4u, mesh2.getAttribute(mesh::Semantic::eWeights).buffer, rhi::Format::eR16G16B16A16_Sfloat, 2u);
                    commandList->bindTexelBuffer(5u, mesh2.getAttribute(mesh::Semantic::eTangent).buffer, rhi::Format::eR8G8B8A8_Snorm, 2u);
                }
            } else {
                ++count;
            }
        }

        if (skeletal) { // dispatch
            dispatchSameMeshes(commandList, frame, currentIt, end, count, offsets.meshesDrawOffset, offsets.skeletonOffset);
            offsets.skeletonOffset += count;
        }
        offsets.meshesDrawOffset += count;

        for (uint32_t m = minId; m <= maxId; ++m) {
            const auto vertexCount = skeletonMeshesVerticesCounts[m];
            skeletonMeshesVerticesCounts[m] = offsets.computeSkinVerticesOffset;
            offsets.computeSkinVerticesOffset += vertexCount;
        }

        for (uint32_t m = 0u; m < j; ++m) {
            memory2[m] = skeletonMeshesVerticesCounts[memory2[m]];
            if (m < n) {
                memory4[m].x = skeletonMeshesVerticesCounts[memory4[m].x];
            }
        }

        _meshTransformIndices->modify(sizeof(uint32_t) * i, sizeof(uint32_t) * startOffset);
        _skeletonIds->modify(sizeof(uint32_t) * j, sizeof(uint32_t) * startSkeletonOffset);

        if (n > 0u) {
            offsets.colorIdSkeletalMeshesOffset += n;
            _colorIdSkeletalMeshesIds->modify(sizeof(vec2u) * n, sizeof(vec2u) * startSkeletalMeshColorIdsOffset);
        }

        if (n1 > 0u) {
            offsets.colorIdMeshesOffset += n1;
            _colorIdMeshesIds->modify(sizeof(vec2u) * n1, sizeof(vec2u) * startMeshColorIdsOffset);
        }
    }

    void Scene::dispatchSameMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        uint32_t count, uint32_t transformIndex, uint32_t skeletonsOffset) {

        constexpr auto kMaxExecutionCount = 0xff'ffu; // 65535u

        struct Uniform {
            uint32_t firstObject;
            uint32_t objectsCount;
	        uint32_t invocationsPerObject;
            uint32_t indicesOffset;
            uint32_t transformsOffset;
            uint32_t skeletonsOffset;
        };

        const auto & mesh = *static_cast<const Mesh*>(*begin);
        const auto & g = mesh.geometry();
        const auto vertexToCompute = g.uniqueIndicesCount;

        // const auto threadsPerObject = (vertexToCompute % kComputeSkinningLocalGroupSize == 0u) ?
        //                                vertexToCompute / kComputeSkinningLocalGroupSize :
        //                                vertexToCompute / kComputeSkinningLocalGroupSize + 1u;

        const auto threadsPerObject = (vertexToCompute + kComputeSkinningLocalGroupSize - 1u) / kComputeSkinningLocalGroupSize;
        
        Uniform uniform {
            .firstObject = 0u,
            .objectsCount = 0u,
            .invocationsPerObject = vertexToCompute,
            .indicesOffset = g.uniqueIndexOffset,
            .transformsOffset = transformIndex,
            .skeletonsOffset = skeletonsOffset
        };

        while (count > 0u) {
            auto const maxCount = std::min(kMaxExecutionCount / threadsPerObject, count);
            uniform.objectsCount = maxCount;

            commandList->pushConstants(rhi::shaders::Stages::eCompute, 0u, sizeof(Uniform), &uniform);

            const auto executeCount = maxCount * threadsPerObject;
            commandList->dispatch(executeCount, 1u, 1u);

            count -= maxCount;
            uniform.firstObject += maxCount;
        }
    }

    Scene::DispatchResult Scene::dispatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, stl::span<AttachmentObject*> objects, const DrawDataOffsets & offsets) {
        if constexpr (!kComputeSkinningAvailable) {
            return {};
        }

        if (objects.empty()) {
            return {};
        }

        DrawDataOffsets offsets2 = offsets;
        auto currentIt = objects.begin();
        uint16_t typeIdx = (*currentIt)->typeIndex;
        for (auto it = currentIt + 1u; it != objects.end(); ++it) {
            const auto currenTypeIndex = (*it)->typeIndex;
            if (currenTypeIndex != typeIdx) {
                dispatchObjects(commandList, frame, currentIt, it, typeIdx, offsets2);

                currentIt = it;
                typeIdx = currenTypeIndex;
            }
        }

        dispatchObjects(commandList, frame, currentIt, objects.end(), typeIdx, offsets2);
        return {
            .computeSkinVerticesOffset = offsets2.computeSkinVerticesOffset
        };
    }

    void Scene::drawObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets) {
        switch(_currentPass) {
        case RenderPass::eCommon:
            drawCommon(commandList, frame, viewParams, begin, end, typeIdx, offsets);
            break;
        case RenderPass::eShadow:
            drawShadows(commandList, frame, viewParams, begin, end, typeIdx, offsets);
            break;
        case RenderPass::eColorId:
            drawColorIds(commandList, frame, viewParams, begin, end, typeIdx, offsets);
            break;
        }
    }

    void Scene::drawCommon(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets) {
        switch (typeIdx) {
        case ctti::typeIndex<AttachmentObject, Skeleton>(): // do not draw skeletons
            break;
        case ctti::typeIndex<AttachmentObject, Mesh>():
        {
            if constexpr (kComputeSkinningAvailable) {
                drawMeshes2(commandList, frame, begin, end, viewParams, offsets.meshesDrawOffset, offsets.skeletonOffset);
            } else {
                drawMeshes(commandList, frame, begin, end, viewParams, offsets.meshesDrawOffset);
            }
        }
            break;
        case ctti::typeIndex<AttachmentObject, Sprite>():
        {
            drawSprites(commandList, frame, begin, end, viewParams, offsets.spriteVerticesOffset, offsets.spriteIndicesOffset);
        }
            break;
        case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
        {
            drawCustomObjects(begin, end, viewParams, commandList, frame);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Dummy>():
        {
            drawDummies(begin, end, viewParams, 0xff'00'00'ffu);
        }
            break;
        }
    }

    void Scene::drawShadows(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets) {
        switch (typeIdx) {
        case ctti::typeIndex<AttachmentObject, Skeleton>(): // do not draw skeletons
            break;
        case ctti::typeIndex<AttachmentObject, Mesh>():
        {
            if constexpr (kComputeSkinningAvailable) {
                drawMeshes2Shadow(commandList, frame, begin, end, viewParams, offsets.meshesDrawOffset, offsets.skeletonOffset);
            } else {
                assert(false);
            }
        }
            break;
        case ctti::typeIndex<AttachmentObject, Sprite>():
            break;
        case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
        {
            drawCustomObjects(begin, end, viewParams, commandList, frame);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Dummy>():
            break;
        }
    }

    void Scene::drawColorIds(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const ViewParams & viewParams, AttachmentsIterator begin, AttachmentsIterator end, 
        uint16_t typeIdx, DrawDataOffsets & offsets) {
        // todo!
        switch (typeIdx) {
        case ctti::typeIndex<AttachmentObject, Skeleton>(): // do not draw skeletons
            break;
        case ctti::typeIndex<AttachmentObject, Mesh>():
        {
            if constexpr (kComputeSkinningAvailable) {
                drawMeshes2ColorIds(commandList, frame, begin, end, viewParams, offsets.colorIdMeshesOffset, offsets.colorIdSkeletalMeshesOffset);
            } else {
                assert(false);
            }
        }
            break;
        case ctti::typeIndex<AttachmentObject, Sprite>():
        {
            // drawSprites2ColorIds(commandList, frame, begin, end, viewParams, offsets.spriteVerticesOffset, offsets.spriteIndicesOffset);
        }
            break;
        case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
        {
            drawCustomObjects(begin, end, viewParams, commandList, frame);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Dummy>():
            break;
        }
    }

    void Scene::drawDebug(const ViewParams & viewParams, stl::span<AttachmentObject*> objects) {
        for (auto object : objects) {
            auto * node = object->node;
            const auto & bounds = node->bounds;
            if (bounds.type == Bounds::Type::eNone) {
                continue;
            }

            uint32_t color = 0xff'00'ff'00u;
            if (object) {
                switch (object->typeIndex) {
                case ctti::typeIndex<AttachmentObject, Skeleton>():
                    color = 0xff'77'ff'00u;
                    break;
                case ctti::typeIndex<AttachmentObject, Mesh>():
                    color = 0xff'77'ff'00u;
                    break;
                case ctti::typeIndex<AttachmentObject, Sprite>():
                    color = 0xff'ff'77'00u;
                    break;
                case ctti::typeIndex<AttachmentObject, CustomSceneObject>():
                    color = 0xff'00'00'ffu;
                    break;
                default:
                    break;
                }
            }

            _drawHelper.drawBounds(bounds, color, viewParams.cameraTransform * node->getWorldTransform());
        }
    }

    void Scene::draw(RenderPass pass, rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const ViewParams & viewParams, stl::span<AttachmentObject*> objects, const DrawDataOffsets & offsets, const void * uniformData) {
        if (objects.empty()) {
            return;
        }

        _currentPass = pass;
        const size_t uniformBufferOffsetAlign = _device->getUniformBufferOffsetAlignment();
        uint32_t uniformBufferSize;

        { // update uniforms
            assert(_currentViewParamsBufferOffset < kViewParamsBufferSizeBytes);

            switch (_currentPass) {
            case RenderPass::eCommon:
            {
                uniformBufferSize = sizeof(ViewParams);
                auto * uniform = _viewParamsUniformBuffer->memory2<std::byte>() + _currentViewParamsBufferOffset;
                memcpy(uniform , &viewParams, uniformBufferSize);
                reinterpret_cast<ViewParams*>(uniform)->time = _shadersTime;
                _viewParamsUniformBuffer->modify(uniformBufferSize, _currentViewParamsBufferOffset);
            }
                break;
            case RenderPass::eShadow:
            {
                assert(uniformData != nullptr);
                uniformBufferSize = sizeof(mat4f) * _shadowCascades;
                auto * uniform = _viewParamsUniformBuffer->memory2<std::byte>() + _currentViewParamsBufferOffset;
                const ShadowViewParams & shadowParams = *static_cast<const ShadowViewParams *>(uniformData);
                _shadowCascades = shadowParams.cascadeMatrices.size();
                memcpy(uniform , shadowParams.cascadeMatrices.data(), uniformBufferSize);
                _viewParamsUniformBuffer->modify(uniformBufferSize, _currentViewParamsBufferOffset);
            }
                break;
            case RenderPass::eColorId:
            {
                assert(uniformData != nullptr);
                uniformBufferSize = sizeof(mat4f);
                auto * uniform = _viewParamsUniformBuffer->memory2<std::byte>() + _currentViewParamsBufferOffset;
                memcpy(uniform , &viewParams.cameraTransform, uniformBufferSize);
                _viewParamsUniformBuffer->modify(uniformBufferSize, _currentViewParamsBufferOffset);
                _colorPassScreenOffset = *static_cast<const vec2f*>(uniformData);
            }
                break;
            default: 
                assert(false);
                uniformBufferSize = 0u;
                break;
            }
        }

        DrawDataOffsets offsets2 = offsets;
        auto currentIt = objects.begin();
        uint16_t typeIdx = (*currentIt)->typeIndex;
        for (auto it = currentIt + 1u; it != objects.end(); ++it) {
            const auto currenTypeIndex = (*it)->typeIndex;
            if (currenTypeIndex != typeIdx) {
                drawObjects(commandList, frame, viewParams, currentIt, it, typeIdx, offsets2);

                currentIt = it;
                typeIdx = currenTypeIndex;
            }
        }

        drawObjects(commandList, frame, viewParams, currentIt, objects.end(), typeIdx, offsets2);

        // change uniform buffer offset
        _currentViewParamsBufferOffset += align_value(uniformBufferSize, uniformBufferOffsetAlign);
    }

    void Scene::drawDummies(AttachmentsIterator begin, AttachmentsIterator end, const ViewParams & viewParams, uint32_t color) {
        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _drawHelper.drawSphere(vec3f{0.0f}, 0.1f, color, 32u, viewParams.cameraTransform * object->node->getWorldTransform());
        }
    }

    void Scene::drawCustomObjects(
        AttachmentsIterator begin, AttachmentsIterator end, 
        const ViewParams & viewParams, rhi::runtime::CommandListPtr & commandList, uint8_t frame
    ) {
        auto currentIt = begin;
        auto & object0 = *static_cast<CustomSceneObject*>(*currentIt);
        auto * delegate = &object0.getDelegate();

        for (auto it = begin + 1; it != end; ++it) {
            auto & object = *static_cast<CustomSceneObject*>(*it);
            auto * delegate2 = &object.getDelegate();

            if (delegate != delegate2) {
                delegate->draw(currentIt, it, *this, viewParams, _currentPass, commandList, frame);
                delegate = delegate2;
                currentIt = it;
            }
        }

        delegate->draw(currentIt, end, *this, viewParams, _currentPass, commandList, frame);
    }

    void Scene::drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const ViewParams & viewParams, uint32_t & vtxOffset, uint32_t & idxOffset) {
        auto currentIt = begin;
        auto it = currentIt;
        const auto & sprite = *static_cast<const Sprite*>(*it);
        uint32_t pipeline = sprite.getPipeline();
        auto image = sprite.getFrame().getImage();
        auto filter = sprite.getFilter();
        ++it;

        uint32_t batchVertexesCount = 4u;
        uint32_t batchIndicesCount = 6u;
        for (; it != end; ++it) {
            const auto & sprite = *static_cast<const Sprite*>(*it);
            const auto & currentImage = sprite.getFrame().getImage();
            const uint32_t currentPipeline = sprite.getPipeline();
            const auto currentFilter = sprite.getFilter();

            const bool completeBatch = currentPipeline != pipeline ||
                                       currentFilter != filter ||
                                       currentImage.get() != image.get();

            if (completeBatch) {
                // draw
                drawSpritesBatch(commandList, frame, currentIt, it, viewParams, batchVertexesCount, batchIndicesCount, vtxOffset, idxOffset);
                vtxOffset += batchVertexesCount;
                idxOffset += batchIndicesCount;

                // begin next batch
                image = currentImage;
                pipeline = currentPipeline;
                filter = currentFilter;
                batchVertexesCount = 4u;
                batchIndicesCount = 6u;
                currentIt = it;
            } else {
                batchVertexesCount += 4u;
                batchIndicesCount += 6u;
            }
        }

        drawSpritesBatch(commandList, frame, currentIt, end, viewParams, batchVertexesCount, batchIndicesCount, vtxOffset, idxOffset);
        vtxOffset += batchVertexesCount;
        idxOffset += batchIndicesCount;
    }

    void Scene::drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const ViewParams & viewParams, uint32_t batchVertexesCount, uint32_t batchIndicesCount, uint32_t vtxOffset, uint32_t idxOffset) {
        using namespace graphics;

        const auto & sprite = *static_cast<const Sprite*>(*begin);
        const auto & pipeline = getGraphicsPipeline(sprite.getPipeline());
        if (pipeline == nullptr) {
            return;
        }
        auto image = sprite.getFrame().getImage();
        // const auto & imageExtent = image->getExtent();
        // vec2f imageSize = {imageExtent.width, imageExtent.height};

        auto * vMem = _spritesVertices->memory2<SpriteVertex>() + vtxOffset;
        auto * iMem = _spritesIndices->memory2<uint32_t>() + idxOffset;

        uint32_t i = 0u, j = 0u;
        for (auto it = begin; it != end; ++it) {
            const auto & sprite = *static_cast<const Sprite*>(*it);
            const auto & frame2D = sprite.getFrame().getFrame();
            const auto & model = sprite.node->getWorldTransform();

            const auto em8 = static_cast<uint8_t>(float_to_unorm(sprite.getEmissive(), 8u));
            const stl::array<uint8_t, 4u> color = {em8, 0u, 0u, 0u};

            const vec2f corner0 = {0.0f, 0.0f};
            const vec2f corner1 = sprite.getSize();
            // const vec2f corner1 = {(frame2D.uvMax[0u] - frame2D.uvMin[0u]) * imageSize.x, 
            //                        (frame2D.uvMax[1u] - frame2D.uvMin[1u]) * imageSize.y};

            // fill vertexes and indices
            // vertexes
            auto v = model * vec4f(corner0.x, corner0.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 0u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMin[0u], frame2D.uvMax[1u]),
                .color = color
            };
            v = model * vec4f(corner1.x, corner0.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 1u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMax[0u], frame2D.uvMax[1u]),
                .color = color
            };
            v = model * vec4f(corner0.x, corner1.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 2u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMin[0u], frame2D.uvMin[1u]),
                .color = color
            };
            v = model * vec4f(corner1.x, corner1.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 3u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMax[0u], frame2D.uvMin[1u]),
                .color = color
            };
            
            // indices
            *(iMem + j + 0u) = vtxOffset + i + 0u;
            *(iMem + j + 1u) = vtxOffset + i + 1u;
            *(iMem + j + 2u) = vtxOffset + i + 2u;

            *(iMem + j + 3u) = vtxOffset + i + 2u;
            *(iMem + j + 4u) = vtxOffset + i + 1u;
            *(iMem + j + 5u) = vtxOffset + i + 3u;

            i += 4u;
            j += 6u;
        }

        _spritesVertices->modify();
        _spritesIndices->modify();

        if (auto & vertexBuffer = (*_spritesVertices)(frame)) { // hmmm, must be after buffer resize!
            auto & indexBuffer = (*_spritesIndices)(frame);
            commandList->bindPipeline(pipeline);
            commandList->bindVertexBuffer(0u, vertexBuffer, 0u);
            commandList->bindIndexBuffer(indexBuffer, 0u, rhi::IndexType::eU32);
            commandList->bindImage(1u, image, sprite.getFilter() == SpriteTextureFilter::eLinear ? _linearSampler : _nearestSampler);

            uint8_t imageSlot = 1u;
            for (const auto & [imgHandler, viewType] : sprite.getSubImages()) {
                if (imgHandler == nullptr) {
                    break;
                }
                commandList->bindImage(++imageSlot, imgHandler->image, 
                    sprite.getFilter() == SpriteTextureFilter::eLinear ? _linearSampler : _nearestSampler, 
                    0u, rhi::ImageAspectFlags::eColor, viewType);
            }

            commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);
            //commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(mat4f), &viewParams.cameraTransform);

            commandList->drawIndexed(batchIndicesCount, 1u, idxOffset);
        }
    }

void Scene::drawMeshes2Shadow(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end, const ViewParams & viewParams, uint32_t & offset, uint32_t & skeletonsOffset) {
        if (getCommonPipeline(CommonPipeline::Type::eMeshShadow) == 0xff'ff'ff'ffu || 
            getCommonPipeline(CommonPipeline::Type::eMeshSkinnedShadow) == 0xff'ff'ff'ffu) {
            return;
        } 

        auto currentIt = begin;
        const auto & mesh = *static_cast<const Mesh*>(*currentIt);
        auto geometryDataIndex = mesh.geometryDataIndex();
        bool skeletal = mesh.getSkeleton() != nullptr;
        auto pipelineIdx = skeletal ? getCommonPipeline(CommonPipeline::Type::eMeshSkinnedShadow) : getCommonPipeline(CommonPipeline::Type::eMeshShadow);
        auto dataIndex = mesh.dataIndex();
        //auto materialIndex = mesh.materialIndex();

        struct Uniform {
            vec4u offset_and_count;
        };

        Uniform uniform {
            .offset_and_count = vec4u{0u}
        };

        uint32_t count = 1u;

        const auto & pipeline = getGraphicsPipeline(pipelineIdx);
        assert(pipeline != nullptr);
        commandList->bindPipeline(pipeline);

        commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);

        if (skeletal) {
            commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
            commandList->bindStorageBuffer(1u, (*_skeletonIds)(frame), 1u, 0u);
        } else {
            commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
            commandList->bindStorageBuffer(1u, (*_meshTransformIndices)(frame), 1u, 0u);
        }

        for (auto it = currentIt + 1u; it != end; ++it) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            
            const auto currentGeometryDataIndex = mesh2.geometryDataIndex();
            const bool currentSkeletal = mesh2.getSkeleton() != nullptr;
            const auto currentPipelineIdx = currentSkeletal ? getCommonPipeline(CommonPipeline::Type::eMeshSkinnedShadow) : getCommonPipeline(CommonPipeline::Type::eMeshShadow);
            const auto currenDataIndex = mesh2.dataIndex();
            //const auto currenMaterialIndex = mesh2.materialIndex();

            const bool pipelineChanged = pipelineIdx != currentPipelineIdx;
            //const bool materialChanged = materialIndex != currenMaterialIndex;

            const bool completePack = pipelineChanged ||
                                //materialChanged ||
                                currentSkeletal != skeletal ||
                                currenDataIndex != dataIndex ||
                                currentGeometryDataIndex != geometryDataIndex;
                 
            if (completePack) {
                // draw
                if (skeletal) {
                    uniform.offset_and_count = {skeletonsOffset, count, 0u, 0u};
                    commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
                    drawSameMeshes(commandList, frame, currentIt, it, count * _shadowCascades, true);
                    skeletonsOffset += count;
                } else {
                    uniform.offset_and_count = {offset, count, 0u, 0u};
                    commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
                    drawSameMeshes(commandList, frame, currentIt, it, count * _shadowCascades, false);
                }
                offset += count;

                if (pipelineChanged) {
                    const auto & pipeline = getGraphicsPipeline(currentPipelineIdx);
                    assert(pipeline != nullptr);
                    commandList->bindPipeline(pipeline);
                    commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);
                }

                if (currentSkeletal != skeletal || pipelineChanged) {
                    if (currentSkeletal) {
                        commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
                        commandList->bindStorageBuffer(1u, (*_skeletonIds)(frame), 1u, 0u);
                    } else {
                        commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
                        commandList->bindStorageBuffer(1u, (*_meshTransformIndices)(frame), 1u, 0u);
                    }
                }

                // begin next pack
                dataIndex = currenDataIndex;
                pipelineIdx = currentPipelineIdx;
                geometryDataIndex = currentGeometryDataIndex;
                skeletal = currentSkeletal;
                //materialIndex = currenMaterialIndex;
                currentIt = it;
                count = 1u;
            } else {
                ++count;
            }
        }

        if (skeletal) {
            uniform.offset_and_count = {skeletonsOffset, count, 0u, 0u};
            commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
            drawSameMeshes(commandList, frame, currentIt, end, count * _shadowCascades, true);
            skeletonsOffset += count;
        } else {
            uniform.offset_and_count = {offset, count, 0u, 0u};
            commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
            drawSameMeshes(commandList, frame, currentIt, end, count * _shadowCascades, false);
        }
        offset += count;
    }

    void Scene::drawMeshes2ColorIds(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        const ViewParams & viewParams, uint32_t & offset, uint32_t & skeletonsOffset) {
        if (getCommonPipeline(CommonPipeline::Type::eMeshColorId) == 0xff'ff'ff'ffu || 
            getCommonPipeline(CommonPipeline::Type::eMeshSkinColorId) == 0xff'ff'ff'ffu) {
            return;
        } 

        struct Uniform {
            vec2f renderOffset;
            uint32_t offset;
        };

        Uniform uniform {
            .renderOffset = _colorPassScreenOffset,
            .offset = 0u
        };

        uint32_t count = 0u;
        auto currentIt = begin;
        const auto & mesh = *static_cast<const Mesh*>(*currentIt);
        auto geometryDataIndex = mesh.geometryDataIndex();
        bool skeletal = mesh.getSkeleton() != nullptr;
        auto pipelineIdx = skeletal ? 
            getCommonPipeline(CommonPipeline::Type::eMeshSkinColorId) : 
            getCommonPipeline(CommonPipeline::Type::eMeshColorId);
        auto dataIndex = mesh.dataIndex();
        bool colorIdEnabled = mesh.isEnableColorId();

        if (colorIdEnabled) {
            count = 1u;
            const auto & pipeline = getGraphicsPipeline(pipelineIdx);
            assert(pipeline != nullptr);
            commandList->bindPipeline(pipeline);
            commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);
            
            if (skeletal) {
                commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
                commandList->bindStorageBuffer(1u, (*_colorIdSkeletalMeshesIds)(frame), 1u, 0u);
            } else {
                commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
                commandList->bindStorageBuffer(1u, (*_colorIdMeshesIds)(frame), 1u, 0u);
            }
        }

        for (auto it = currentIt + 1u; it != end; ++it) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            if (!mesh2.isEnableColorId()) {
                continue;
            }
            
            const auto currentGeometryDataIndex = mesh2.geometryDataIndex();
            const bool currentSkeletal = mesh2.getSkeleton() != nullptr;
            const auto currentPipelineIdx = currentSkeletal ? 
                getCommonPipeline(CommonPipeline::Type::eMeshSkinColorId) : 
                getCommonPipeline(CommonPipeline::Type::eMeshColorId);
            const auto currenDataIndex = mesh2.dataIndex();

            if (count == 0u) {
                const auto & pipeline = getGraphicsPipeline(currentPipelineIdx);
                assert(pipeline != nullptr);
                commandList->bindPipeline(pipeline);
                commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);
                
                if (currentSkeletal) {
                    commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
                    commandList->bindStorageBuffer(1u, (*_colorIdSkeletalMeshesIds)(frame), 1u, 0u);
                } else {
                    commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
                    commandList->bindStorageBuffer(1u, (*_colorIdMeshesIds)(frame), 1u, 0u);
                }

                pipelineIdx = currentPipelineIdx;
                skeletal = currentSkeletal;
                dataIndex = currenDataIndex;
                geometryDataIndex = currentGeometryDataIndex;
                currentIt = it;
            }

            const bool pipelineChanged = pipelineIdx != currentPipelineIdx;
            const bool completePack = pipelineChanged ||
                                      currentSkeletal != skeletal ||
                                      currenDataIndex != dataIndex ||
                                      currentGeometryDataIndex != geometryDataIndex;
                 
            if (completePack) {
                // draw
                if (colorIdEnabled) {
                    if (skeletal) {
                        uniform.offset = skeletonsOffset;
                        commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
                        drawSameMeshes(commandList, frame, currentIt, it, count, true);
                    } else {
                        uniform.offset = offset;
                        commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
                        drawSameMeshes(commandList, frame, currentIt, it, count, false);
                    }
                }

                if (skeletal) {
                    skeletonsOffset += count;
                } else {
                    offset += count;
                }

                if (pipelineChanged) {
                    const auto & pipeline = getGraphicsPipeline(currentPipelineIdx);
                    assert(pipeline != nullptr);
                    commandList->bindPipeline(pipeline);
                    commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);
                }

                if (currentSkeletal != skeletal || pipelineChanged) {
                    if (currentSkeletal) {
                        commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
                        commandList->bindStorageBuffer(1u, (*_colorIdSkeletalMeshesIds)(frame), 1u, 0u);
                    } else {
                        commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
                        commandList->bindStorageBuffer(1u, (*_colorIdMeshesIds)(frame), 1u, 0u);
                    }
                }

                // begin next pack
                dataIndex = currenDataIndex;
                pipelineIdx = currentPipelineIdx;
                geometryDataIndex = currentGeometryDataIndex;
                skeletal = currentSkeletal;
                currentIt = it;
                count = 1u;
            } else {
                ++count;
            }

            colorIdEnabled = true;
        }

        if (!colorIdEnabled) {
            return;
        }

        if (skeletal) {
            uniform.offset = skeletonsOffset;
            commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
            drawSameMeshes(commandList, frame, currentIt, end, count, true);
            skeletonsOffset += count;
        } else {
            uniform.offset = offset;
            commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
            drawSameMeshes(commandList, frame, currentIt, end, count, false);
            offset += count;
        }
    }

    void Scene::drawMeshes2(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end, const ViewParams & viewParams, uint32_t & offset, uint32_t & skeletonsOffset) {
        auto currentIt = begin;
        const auto & mesh = *static_cast<const Mesh*>(*currentIt);
        auto pipelineIdx = mesh.getPipeline();
        auto geometryDataIndex = mesh.geometryDataIndex();
        bool skeletal = mesh.getSkeleton() != nullptr;
        auto dataIndex = mesh.dataIndex();
        auto materialIndex = mesh.materialIndex();

        struct Uniform {
            vec4f color;
            vec4f emissive;
            vec4f metalicRoughnessAlphaCut;
            vec4u offset;
        };

        Uniform uniform {
            .color = vec4f{1.0f},
            .emissive = vec4f{0.0f},
            .metalicRoughnessAlphaCut = vec4f{0.0f},
            .offset = vec4u{0u}
        };

        auto applyMaterial = [this, &commandList, &uniform](uint16_t materialIndex) noexcept {
            const auto & material = _graphicsService.getMaterialManager().getMaterial(materialIndex);
            const auto & textures = material.textures;

            const auto & image = textures[static_cast<uint8_t>(graphics::PbrTextureId::eBaseColor)];
            assert(image != nullptr); // check textureManager default resources generated!

            const auto & image2 = textures[static_cast<uint8_t>(graphics::PbrTextureId::eEmissive)];
            assert(image2 != nullptr);

            const auto & image3 = textures[static_cast<uint8_t>(graphics::PbrTextureId::eMetallicRoughness)];
            assert(image3 != nullptr);

            const auto & image4 = textures[static_cast<uint8_t>(graphics::PbrTextureId::eNormal)];
            assert(image4 != nullptr);

            // textures change more often than storage buffers - setId #2u used
            commandList->bindImage(0u, image->image, _linearSampler, 2u); 
            commandList->bindImage(1u, image2->image, _linearSampler, 2u);
            commandList->bindImage(2u, image3->image, _linearSampler, 2u);
            commandList->bindImage(3u, image4->image, _linearSampler, 2u);
            uniform.color = material.baseColorFactor;
            uniform.emissive = vec4f{material.emissiveFactor, 0.0f};
            uniform.metalicRoughnessAlphaCut = vec4f(material.metallicFactor, material.roughnessFactor, material.alphaCutoff, 1.0f);
        };

        uint32_t count = 1u;

        const auto & pipeline = getGraphicsPipeline(pipelineIdx);
        assert(pipeline != nullptr);
        
        commandList->bindPipeline(pipeline);
        commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);

        if (skeletal) {
            commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
            commandList->bindStorageTexelBuffer(1u, _computedNormals[frame], rhi::Format::eR8G8B8A8_Snorm, 1u, 0u);
            commandList->bindStorageTexelBuffer(2u, _computedTangents[frame], rhi::Format::eR8G8B8A8_Snorm, 1u, 0u);
            commandList->bindStorageBuffer(3u, (*_skeletonIds)(frame), 1u, 0u);
        } else {
            commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
            commandList->bindStorageBuffer(1u, (*_meshTransformIndices)(frame), 1u, 0u);
        }

        uniform.offset.x = skeletal ? skeletonsOffset : offset;

        applyMaterial(materialIndex);
        commandList->pushConstants(rhi::shaders::Stages::eVertex | rhi::shaders::Stages::eFragment, 0u, sizeof(Uniform), &uniform);

        for (auto it = currentIt + 1u; it != end; ++it) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            
            const auto currentGeometryDataIndex = mesh2.geometryDataIndex();
            const bool currentSkeletal = mesh2.getSkeleton() != nullptr;
            const auto currentPipelineIdx = mesh2.getPipeline();
            const auto currenDataIndex = mesh2.dataIndex();
            const auto currenMaterialIndex = mesh2.materialIndex();

            const bool pipelineChanged = pipelineIdx != currentPipelineIdx;
            const bool materialChanged = materialIndex != currenMaterialIndex;

            const bool completePack = pipelineChanged ||
                                materialChanged ||
                                currentSkeletal != skeletal ||
                                currenDataIndex != dataIndex ||
                                currentGeometryDataIndex != geometryDataIndex;
                 
            if (completePack) {
                // draw
                if (skeletal) {
                    drawSameMeshes(commandList, frame, currentIt, it, count, true/*skeletal*/);
                    skeletonsOffset += count;
                } else {
                    drawSameMeshes(commandList, frame, currentIt, it, count, false/*skeletal*/);
                }

                offset += count;

                uniform.offset.x = currentSkeletal ? skeletonsOffset : offset;

                if (pipelineChanged) {
                    const auto & pipeline = getGraphicsPipeline(currentPipelineIdx);
                    assert(pipeline != nullptr);
                    commandList->bindPipeline(pipeline);
                    commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);
                }

                if (pipelineChanged || materialChanged) {
                    applyMaterial(currenMaterialIndex);
                    commandList->pushConstants(rhi::shaders::Stages::eVertex | rhi::shaders::Stages::eFragment, 
                        0u, sizeof(Uniform), &uniform);
                } else {
                    commandList->pushConstants(rhi::shaders::Stages::eVertex | rhi::shaders::Stages::eFragment, 
                        offset_of(&Uniform::offset), sizeof(vec4u), &uniform.offset);
                }

                if (currentSkeletal != skeletal || pipelineChanged) {
                    if (currentSkeletal) {
                        commandList->bindStorageTexelBuffer(0u, _computedVertices[frame], rhi::Format::eR32G32B32A32_Sfloat, 1u, 0u);
                        commandList->bindStorageTexelBuffer(1u, _computedNormals[frame], rhi::Format::eR8G8B8A8_Snorm, 1u, 0u);
                        commandList->bindStorageTexelBuffer(2u, _computedTangents[frame], rhi::Format::eR8G8B8A8_Snorm, 1u, 0u);
                        commandList->bindStorageBuffer(3u, (*_skeletonIds)(frame), 1u, 0u);
                    } else {
                        commandList->bindStorageBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
                        commandList->bindStorageBuffer(1u, (*_meshTransformIndices)(frame), 1u, 0u);
                    }
                }

                // begin next pack
                dataIndex = currenDataIndex;
                pipelineIdx = currentPipelineIdx;
                geometryDataIndex = currentGeometryDataIndex;
                skeletal = currentSkeletal;
                materialIndex = currenMaterialIndex;
                currentIt = it;
                count = 1u;
            } else {
                ++count;
            }
        }

        if (skeletal) {
            drawSameMeshes(commandList, frame, currentIt, end, count, true/*skeletal*/);
            skeletonsOffset += count;
        } else {
            drawSameMeshes(commandList, frame, currentIt, end, count, false/*skeletal*/);
        }
        offset += count;
    }

    void Scene::drawMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        AttachmentsIterator begin, AttachmentsIterator end, const ViewParams & viewParams, uint32_t & offset) {
        auto currentIt = begin;
        auto it = currentIt;
        const auto & mesh = *static_cast<const Mesh*>(*it);
        auto pipelineIdx = mesh.getPipeline();
        auto geometryDataIndex = mesh.geometryDataIndex();
        bool skeletal = mesh.getSkeleton() != nullptr;
        auto dataIndex = mesh.dataIndex();
        auto materialIndex = mesh.materialIndex();

        const auto startOffset = offset;
        uint32_t i = 0u;
        auto * memory = _meshTransformIndices->memory2<uint32_t>() + offset;
        if (skeletal) {
            memory[i] = _meshTransformIndicesMap[*it].skeletalTransformIndex;
        } else {
            memory[i] = _meshTransformIndicesMap[*it].transformIndex;
        }
        ++it;
        ++i;

        struct Uniform {
            vec4f color;
            vec4f emissive;
        };

        Uniform uniform {
            .color = vec4f{1.0f},
            .emissive = vec4f{0.0f}
        };

        auto applyMaterial = [this, &commandList, &uniform](uint16_t materialIndex) noexcept {
            const auto & material = _graphicsService.getMaterialManager().getMaterial(materialIndex);
            const auto & textures = material.textures;

            const auto & image = textures[static_cast<uint8_t>(graphics::PbrTextureId::eBaseColor)];
            assert(image != nullptr); // check textureManager default resources generated!

            const auto & image2 = textures[static_cast<uint8_t>(graphics::PbrTextureId::eEmissive)];
            assert(image2 != nullptr);

            // textures change more often than storage buffers - setId #2u used
            commandList->bindImage(0u, image->image, _linearSampler, 2u);
            commandList->bindImage(1u, image2->image, _linearSampler, 2u); 
            uniform.color = material.baseColorFactor;
            uniform.emissive = vec4f{material.emissiveFactor, 0.0f};
        };

        uint32_t count = 1u;

        const auto & pipeline = getGraphicsPipeline(pipelineIdx);
        assert(pipeline != nullptr);
        commandList->bindPipeline(pipeline);
        
        commandList->bindBuffer(0u, (*_viewParamsUniformBuffer)(frame), 0u, _currentViewParamsBufferOffset);

        applyMaterial(materialIndex);
        commandList->pushConstants(rhi::shaders::Stages::eVertex | rhi::shaders::Stages::eFragment, 0u, sizeof(Uniform), &uniform);

        if (skeletal) {
            commandList->bindBuffer(0u, (*_meshSkeletalTransforms)(frame), 1u, 0u);
        } else {
            commandList->bindBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
        }
        
        for (; it != end; ++it, ++i) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            
            const auto currentGeometryDataIndex = mesh2.geometryDataIndex();
            const bool currentSkeletal = mesh2.getSkeleton() != nullptr;
            const auto currentPipelineIdx = mesh2.getPipeline();
            const auto currenDataIndex = mesh2.dataIndex();
            const auto currenMaterialIndex = mesh2.materialIndex();

            if (currentSkeletal) {
                memory[i] = _meshTransformIndicesMap[*it].skeletalTransformIndex;
            } else {
                memory[i] = _meshTransformIndicesMap[*it].transformIndex;
            }

            const bool pipelineChanged = pipelineIdx != currentPipelineIdx;
            const bool materialChanged = materialIndex != currenMaterialIndex;

            const bool completePack = pipelineChanged ||
                                materialChanged ||
                                currentSkeletal != skeletal ||
                                currenDataIndex != dataIndex ||
                                currentGeometryDataIndex != geometryDataIndex;
                 
            if (completePack) {
                // draw
                drawSameMeshes(commandList, frame, currentIt, it, count, skeletal);
                offset += count;

                if (pipelineChanged) {
                    const auto & pipeline = getGraphicsPipeline(currentPipelineIdx);
                    assert(pipeline != nullptr);
                    commandList->bindPipeline(pipeline);
                }

                if (pipelineChanged || materialChanged) {
                    applyMaterial(currenMaterialIndex);
                    commandList->pushConstants(rhi::shaders::Stages::eVertex | rhi::shaders::Stages::eFragment, 0u, sizeof(Uniform), &uniform);
                }

                if (currentSkeletal != skeletal || pipelineChanged) {
                    if (currentSkeletal) {
                        commandList->bindBuffer(0u, (*_meshSkeletalTransforms)(frame), 1u, 0u);
                    } else {
                        commandList->bindBuffer(0u, (*_meshTransforms)(frame), 1u, 0u);
                    }
                }

                // begin next pack
                dataIndex = currenDataIndex;
                pipelineIdx = currentPipelineIdx;
                geometryDataIndex = currentGeometryDataIndex;
                skeletal = currentSkeletal;
                materialIndex = currenMaterialIndex;
                currentIt = it;
                count = 1u;
            } else {
                ++count;
            }
        }

        drawSameMeshes(commandList, frame, currentIt, end, count, skeletal);
        offset += count;

        _meshTransformIndices->modify(sizeof(uint32_t) * i, sizeof(uint32_t) * startOffset);
    }

    void Scene::drawSameMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end,
        uint32_t count, bool skeletal) {
        const auto & mesh = *static_cast<const Mesh*>(*begin);

        if constexpr (kComputeSkinningAvailable) {
            if (skeletal) { // skinning with compute shader
                uint32_t binding_v = 0u;
                if (_currentPass == RenderPass::eCommon) {
                    commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eUv0).buffer, 0u);
                }
                commandList->bindIndexBuffer(mesh.getIndices().buffer, 0u, rhi::IndexType::eU32);

                const auto & g = mesh.geometry();
                commandList->drawIndexed(g.indexCount, count, g.firstIndex);
                return;
            }
        }

        uint32_t binding_v = 0u;
        commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::ePosition).buffer, 0u);
        if (_currentPass == RenderPass::eCommon) {
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eNormal).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eUv0).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eTangent).buffer, 0u);
        }

        if (skeletal) { // skeletalMesh
            // skinning in vertex shader impl
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eJoints).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eWeights).buffer, 0u);
        }

        commandList->bindIndexBuffer(mesh.getIndices().buffer, 0u, rhi::IndexType::eU32);

        const auto & geometry = mesh.geometry();
        commandList->drawIndexed(geometry.indexCount, count, geometry.firstIndex);
    }

    void Scene::reduceUsedMemory(std::chrono::time_point<std::chrono::steady_clock> now) noexcept {
        const std::chrono::duration<float> duration = now - _tryReduceUsedMemoryTime; // as default in seconds
		const auto elapsed = duration.count();

        if (elapsed >= kTryReduceUsedMemoryTimePeriodSeconds) {
            _tryReduceUsedMemoryTime = now;

            {
                auto const minCount = std::max(_spritesVertexesCount, kInitialVerticesCountMax);
                if (minCount * 2u < _lastFramesUsedSpritesVertexCount) {
                    _spritesVertices->resize(minCount * kSpriteVertexSize);
                }
                _lastFramesUsedSpritesVertexCount = _spritesVertexesCount;
            }

            {
                auto const minCount = std::max(_spritesIndicesCount, kInitialIndicesCountMax);
                if (minCount * 2u < _lastFramesUsedSpritesIndicesCount) {
                    _spritesIndices->resize(minCount * sizeof(uint32_t));
                }
                _lastFramesUsedSpritesIndicesCount = _spritesIndicesCount;
            }

            {
                auto const minCount = std::max(_meshTransformsInFrame, kInitialMeshTransformsInFrameMax);
                if (minCount * 2u < _lastFramesUsedMeshsTransforms) {
                    _meshTransforms->resize(minCount * kSsboTransformSize);
                    _tmpUniqueMeshes.resize(minCount);
                    _tmpUniqueMeshes.shrink_to_fit(); // ?
                }
                _lastFramesUsedMeshsTransforms = _meshTransformsInFrame;
            }

            {
                auto const minCount = std::max(_skeletalTransformsInFrame, kInitialSkeletalmeshTransformsInFrameMax * kDefaultSkeletoneBonesCount);
                if (minCount * 2u < _lastFramesUsedSkeletalTransforms) {
                    _meshSkeletalTransforms->resize(minCount * kSsboTransformSize);
                }
                _lastFramesUsedSkeletalTransforms = _skeletalTransformsInFrame;
            }

            {
                auto const minCount = std::max(_meshesDrawInFrame, kInitialMeshTransformsInFrameMax);
                if (minCount * 2u < _lastFramesUsedMeshesDraw) {
                    _meshTransformIndices->resize(minCount * sizeof(uint32_t));
                }
                _lastFramesUsedMeshesDraw = _meshesDrawInFrame;

                auto const minCount2 = std::max(_skeletonsInFrame, kInitialMeshTransformsInFrameMax);
                if (minCount2 * 2u < _lastFramesUsedSkeletonsInFrame) {
                    _skeletonIds->resize(minCount2 * sizeof(uint32_t));
                }
                _lastFramesUsedSkeletonsInFrame = _skeletonsInFrame;
            }

            {
                auto const minCount = std::max(_colorIdMeshesIndicesInFrame, kInitialMeshColorIdsInFrameMax);
                if (minCount * 2u < _lastFramesUsedColorIdMeshesIndicesInFrame) {
                    _colorIdMeshesIds->resize(minCount * sizeof(vec2u));
                }
                _lastFramesUsedColorIdMeshesIndicesInFrame = _colorIdMeshesIndicesInFrame;
            }

            {
                auto const minCount = std::max(_colorIdSkeletalMeshesIndicesInFrame, kInitialMeshColorIdsInFrameMax);
                if (minCount * 2u < _lastFramesUsedColorIdSkeletalMeshesIndicesInFrame) {
                    _colorIdSkeletalMeshesIds->resize(minCount * sizeof(vec2u));
                }
                _lastFramesUsedColorIdSkeletalMeshesIndicesInFrame = _colorIdSkeletalMeshesIndicesInFrame;
            }

            // for staging buffer
            if (_staging) {
                auto const stagingTargetSize = std::max(calculateTargetStagingSize() * kStaginBufferEnlargeSizeMult, kMinStagingSize);
                auto const stagingCurrentSize = _staging->sizeInBytes();
                if (stagingTargetSize < stagingCurrentSize) {
                    _staging = std::make_unique<graphics::InflightStagingBuffer>(*_api, *_device, stagingTargetSize, 
                        rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
                }
            }
        } else {
            if (_spritesVertexesCount > _lastFramesUsedSpritesVertexCount || 
                _spritesIndicesCount > _lastFramesUsedSpritesIndicesCount ||
                _meshTransformsInFrame > _lastFramesUsedMeshsTransforms ||
                _meshesDrawInFrame > _lastFramesUsedMeshesDraw ||
                _skeletalTransformsInFrame > _lastFramesUsedSkeletalTransforms ||
                _skeletonsInFrame > _lastFramesUsedSkeletonsInFrame ||
                _colorIdMeshesIndicesInFrame > _lastFramesUsedColorIdMeshesIndicesInFrame ||
                _colorIdSkeletalMeshesIndicesInFrame > _lastFramesUsedColorIdSkeletalMeshesIndicesInFrame
            ) {
                _lastFramesUsedSpritesVertexCount = _spritesVertexesCount;
                _lastFramesUsedSpritesIndicesCount = _spritesIndicesCount;
                _lastFramesUsedMeshsTransforms = _meshTransformsInFrame;
                _lastFramesUsedMeshesDraw = _meshesDrawInFrame;
                _lastFramesUsedSkeletalTransforms = _skeletalTransformsInFrame;
                _lastFramesUsedSkeletonsInFrame = _skeletonsInFrame;
                _lastFramesUsedColorIdMeshesIndicesInFrame = _colorIdMeshesIndicesInFrame;
                _lastFramesUsedColorIdSkeletalMeshesIndicesInFrame = _colorIdSkeletalMeshesIndicesInFrame;
                _tryReduceUsedMemoryTime = now;
            }
        }
    }

    void Scene::completeAnimations() noexcept {
        for (auto * taskW8 : _calculationWaiterPtrs.view()) {
            taskW8->request_wait_and_reset();
        }
        _calculationWaiterPtrs.clear();
        _animations.clearAndReserve();
    }

    void Scene::beginFrame(std::chrono::time_point<std::chrono::steady_clock> now) noexcept {
        reduceUsedMemory(now);
        _currentViewParamsBufferOffset = 0u;

        _meshTransformIndicesMap.clear();
        _tmpUniqueMeshes.clear();
        _meshTransformIndicesMap.reserve(_meshTransformsInFrame);
    
        _spritesVertexesCount = 0u;
        _spritesIndicesCount = 0u;
        _meshTransformsInFrame = 0u;
        _meshesDrawInFrame = 0u;
        _skeletalTransformsInFrame = 0u;
        _skeletonsInFrame = 0u;
        _uniqueSkeletons = 0u;
        _colorIdMeshesIndicesInFrame = 0u;
        _colorIdSkeletalMeshesIndicesInFrame = 0u;

        for (auto * delegate : _customDelegates) {
            delegate->beginFrame(now);
        }

        completeAnimations();

        const std::chrono::duration<float> seconds = now.time_since_epoch();
        _shadersTime = fract(seconds.count() * _animationSpeed * kShaderTimeMultiplier);

        ++_frameNumber;
    }

    void Scene::endFrame(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame) {
        for (auto * delegate : _customDelegates) {
            delegate->endFrame(commandList, transferCommandList, frame);
        }

        performCommits(transferCommandList, frame);
    }

    uint32_t Scene::calculateTargetStagingSize() const noexcept { 
        constexpr auto bufferTargetSize = [](const graphics::InflightBuffer & buffer) noexcept {
            const auto size = buffer.sizeInBytes();
            return buffer.isHostVisible() ? 0u : (size < rhi::runtime::command_limits::kMaxMemoryBytesToCopyCommand ? 0u : size);
        };

        return bufferTargetSize(*_spritesVertices) + bufferTargetSize(*_spritesIndices) + 
               bufferTargetSize(*_meshTransforms) + bufferTargetSize(*_meshSkeletalTransforms) + 
               bufferTargetSize(*_meshTransformIndices) + bufferTargetSize(*_skeletonIds) +
               bufferTargetSize(*_colorIdMeshesIds) + bufferTargetSize(*_colorIdSkeletalMeshesIds);
    }

    void Scene::performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        _viewParamsUniformBuffer->commit(*commandList, frame);
        _spritesVertices->commit(*commandList, frame);
        _spritesIndices->commit(*commandList, frame);
        _meshTransforms->commit(*commandList, frame);
        _meshSkeletalTransforms->commit(*commandList, frame);
        _meshTransformIndices->commit(*commandList, frame);
        _skeletonIds->commit(*commandList, frame);
        _colorIdMeshesIds->commit(*commandList, frame);
        _colorIdSkeletalMeshesIds->commit(*commandList, frame);
    }
}