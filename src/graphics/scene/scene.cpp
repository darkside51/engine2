#include "scene.h"
#include "animation.h"
#include "animation_manager.h"
#include "animation/texture/frame_sequence_animation.h"
#include "objects/sprite.h"
#include "objects/mesh.h"

#include "graphics/common/graphics_service.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/utils/vertex_processing.h"

#include "threads/thread_pool/thread_pool.h"

#include "graphics/common/material.h"
#include "graphics/mesh/model_manager.h"
#include "graphics/texture/texture_manager.h"

#include <algorithm>

#include <unordered_map>

namespace engine::scene {
    constexpr float kTryReduceUsedMemoryTimePeriodSeconds = 5.0f;
    constexpr uint32_t kInitialVerticesCountMax = 1024u;
    constexpr uint32_t kInitialIndicesCountMax = 1024u;
    constexpr uint32_t kInitialmeshTransformsInFrameMax = 128u;
    constexpr uint32_t kInitialSkeletalmeshTransformsInFrameMax = 128u;
    constexpr uint32_t kMinStagingSize = 100u * 1024u /*100 Kb*/;
    constexpr uint32_t kStaginBufferEnlargeSizeMult = 2u;

    constexpr uint16_t kUndefinedAnimId = 0xff'ffu;

    using SsboTransform = mat3x4f; // to use 48bytes transposed matrices
    constexpr uint32_t kSsboTransformSize = sizeof(SsboTransform);
    constexpr uint32_t kDefaultSkeletoneBonesCount = 128u;
    constexpr uint32_t kSsboSkeletalTransformSize = sizeof(SsboTransform) * kDefaultSkeletoneBonesCount;

    FramesSequenceAnimator gSequenceAnimator{1.0f / 30.0f};
    SkeletalAnimator gSkeletalAnimator{};

    constexpr uint8_t kSkeletalAnimationCountToEnableMultyThreading = 64u;
    constexpr uint8_t kAnimationCalculationPoolsCount = 4u;

    threads::ThreadPool gThreadPool{kAnimationCalculationPoolsCount};
    std::array<SkeletalAnimator, kAnimationCalculationPoolsCount> gSkeletalAnimatorsMT;

    // struct ColoredVertex {
    //     std::array<float, 3u> coord;
    //     uint32_t color;
    // };
    
    // struct TexturedVertex {
    //     std::array<float, 3u> coord;
    //     std::array<float, 2u> uv;
    // };

     struct SpriteVertex {
        std::array<float, 3u> position;
        std::array<uint16_t, 2u> uv;
    };

    constexpr uint32_t kSpriteVertexSize = sizeof(SpriteVertex);

    void attachObjectToNode(AttachmentObject & object, Node & node) noexcept {
        switch(object.typeIndex) {
        case ctti::typeIndex<AttachmentObject, Skeleton>(): [[fallthrough]];
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

    Scene::Scene(const graphics::GraphicsService & graphicsService, debug::DebugDraw & drawHelper) : 
        _rootNode(createNode()),
        _graphicsService(graphicsService),
        _drawHelper(drawHelper) {
        registerTypes();
    }

    Scene::~Scene() {
        destroy();

        // for (auto & [_, objects] : _objectCollections) {
        // }
    }

    void Scene::destroy() noexcept {
        destroyNode(_rootNode);
        _rootNode = nullptr;

        _animations.clear();

        _spritesVertices = nullptr;
        _spritesIndices = nullptr;

        _meshTransformIndices = nullptr;
        _meshTransforms = nullptr;
        _meshSkeletalTransforms = nullptr;
        
        _staging = nullptr;
        _pipelines.clear();
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

    bool Scene::destroyAnimInfo(AnimationInfoChain * info) noexcept {
        const bool result = (info != nullptr);
        while (info) {
            auto next = info->next;
            _animationInfosAllocator.destroy(info);
            info = next;
        }
        return result;
    }

    uint32_t Scene::registerPipeline(rhi::runtime::PipelinePtr && pipeline) {
        if (pipeline) {
            _pipelines.push_back(std::move(pipeline));
            return _pipelines.size() - 1u;
        }

        return 0xff'ff'ff'ffu;
    }

    const rhi::runtime::PipelinePtr & Scene::getPipeline(uint32_t pipelineId) const noexcept {
        if (pipelineId >= _pipelines.size()) {
            static constexpr rhi::runtime::PipelinePtr kNullPipeline = nullptr;
            return kNullPipeline;
        }
        return _pipelines[pipelineId];
    }

    void Scene::initRenderData(rhi::runtime::Rhi & api, rhi::runtime::Device & device) {
        _api = &api;
        _device = &device;

        // samplers
        _linearSampler.info.magFilter = rhi::FilterMode::eLinear;
        _linearSampler.info.minFilter = rhi::FilterMode::eLinear;
        // _linearSampler.info.addressModeU = rhi::SamplerAddressMode::eClamp_To_Edge;
        // _linearSampler.info.addressModeV = rhi::SamplerAddressMode::eClamp_To_Edge;
        // _linearSampler.info.addressModeW = rhi::SamplerAddressMode::eClamp_To_Edge;
        _linearSampler.info.addressModeU = rhi::SamplerAddressMode::eRepeat;
        _linearSampler.info.addressModeV = rhi::SamplerAddressMode::eRepeat;
        _linearSampler.info.addressModeW = rhi::SamplerAddressMode::eRepeat;
        
        _nearestSampler.info.magFilter = rhi::FilterMode::eNearest;
        _nearestSampler.info.minFilter = rhi::FilterMode::eNearest;
        _nearestSampler.info.addressModeU = rhi::SamplerAddressMode::eRepeat;
        _nearestSampler.info.addressModeV = rhi::SamplerAddressMode::eRepeat;
        _nearestSampler.info.addressModeW = rhi::SamplerAddressMode::eRepeat;

        device.evaluateSampler(_nearestSampler);
        device.evaluateSampler(_linearSampler);

        //create vertex and index buffers
        const rhi::BufferInfo verticesInfo {
            rhi::BufferUsageFlags::eVertex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        const rhi::BufferInfo indicesInfo {
            rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
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
            api, device, storageInfo, kSsboTransformSize * kInitialmeshTransformsInFrameMax
        );

        _meshSkeletalTransforms = std::make_unique<graphics::InflightBuffer>(
            api, device, storageInfo, kSsboSkeletalTransformSize * kInitialSkeletalmeshTransformsInFrameMax
        );

        _meshTransformIndices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, sizeof(uint32_t) * kInitialmeshTransformsInFrameMax
        );
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
        case ctti::typeIndex<AttachmentObject, Dummy>():
            return static_cast<const Dummy &>(a).cmp(static_cast<const Dummy &>(b));
        }

        return false;
    }

    void Scene::update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, std::span<AttachmentObject*> objects, 
        const AnimationManager & animationManager) {
        if (objects.empty()) {
            return;
        }

        // static thread_local std::vector<animation::AnimatedObject> sequences; // todo: remove static
        
        const auto skeletalAnimationsCountOnStart = _animations.skeletalAnimations.size();
        const auto sequenceAnimationsCountOnStart = _animations.sequences.size();

        for (auto * object : objects) {
            const auto * animInfo = object->animInfo;
            while (animInfo) {
                if (animInfo->info.weight > 0.0f) {
                    switch (animInfo->info.animationType) {
                    case ctti::typeIndex<Animation, SkeletalAnimation>() : 
                    {
                        _animations.skeletalAnimations.emplace_back(&animInfo->info, object);
                    }
                        break;
                    case ctti::typeIndex<Animation, FramesSequence>() : 
                    {
                        _animations.sequences.emplace_back(&animInfo->info, object);
                    }
                        break;
                    default:
                        break;
                    }
                }
                animInfo = animInfo->next;
            }
        }

        const std::chrono::duration<float> nowDuration = now.time_since_epoch();
        const float nowTimeCount = nowDuration.count();

        if (!_animations.skeletalAnimations.empty()) { // skeletal animations
            auto begin = _animations.skeletalAnimations.begin() + skeletalAnimationsCountOnStart;
            std::sort(begin, _animations.skeletalAnimations.end(), [](const auto & seq0, const auto & seq1) {
                        return seq0.animInfo->animationId < seq1.animInfo->animationId;
                    });

            auto animationsCount = static_cast<uint32_t>(_animations.skeletalAnimations.size() - skeletalAnimationsCountOnStart);

            if (animationsCount > kSkeletalAnimationCountToEnableMultyThreading) {
                const auto numWorkers = static_cast<uint32_t>(gThreadPool.numWorkers());
                const auto taskPerThread = std::max(animationsCount / numWorkers, 1u);
                
                auto it = begin;
                size_t i = 0u;
                while (animationsCount > 0u) {
                    const auto count = (i == numWorkers - 1u) ? animationsCount : taskPerThread;
                    const auto animations = std::span{it, it + count};
                    it += count;
                    
                    const auto w8Id = _calculationWaiterPtrs.size();
                    auto * taskW8 = _calculationWaiterPtrs.emplace_back(&_calculationWaiters[w8Id]);

                    gThreadPool.execute([animator = &gSkeletalAnimatorsMT[i], &animationManager, nowTimeCount, animations, taskW8]() {
                        auto animId = kUndefinedAnimId;
                        const SkeletalAnimation * currentAnim = nullptr;

                        for (const auto & [animInfo, object] : animations) {
                            if (animInfo->animationId != animId) {
                                animId = animInfo->animationId;
                                currentAnim = animationManager.getAnimation<SkeletalAnimation>(animId);
                            }

                            if (currentAnim) {
                                auto * skeleton = static_cast<Skeleton*>(object);
                                animator->enlarge2(skeleton->nodesCount());
                                animator->update2(nowTimeCount - animInfo->startTime, *currentAnim);
                                animator->apply2(*skeleton);
                            }
                        }

                        taskW8->request_complete();
                    });

                    animationsCount -= count;
                    ++i;
                }
            } else {
                auto animId = kUndefinedAnimId;
                const SkeletalAnimation * currentAnim = nullptr;

                for (const auto & [animInfo, object] : _animations.skeletalAnimations) {
                    if (animInfo->animationId != animId) {
                        animId = animInfo->animationId;
                        currentAnim = animationManager.getAnimation<SkeletalAnimation>(animId);
                    }

                    if (currentAnim) {
                        auto * skeleton = static_cast<Skeleton*>(object);
                        gSkeletalAnimator.enlarge2(skeleton->nodesCount());
                        gSkeletalAnimator.update2(nowTimeCount - animInfo->startTime, *currentAnim);
                        gSkeletalAnimator.apply2(*skeleton);
                    }
                }
            }
        }

        if (!_animations.sequences.empty()) { // sequences
            auto begin = _animations.sequences.begin() + sequenceAnimationsCountOnStart;
            std::sort(begin, _animations.sequences.end(), [](const auto & seq0, const auto & seq1) {
                        return seq0.animInfo->animationId < seq1.animInfo->animationId;
                    });

            auto animId = kUndefinedAnimId;
            const FramesSequence * currentAnim = nullptr;
            
            const auto animations = std::span{begin, _animations.sequences.end()};
            for (const auto & [animInfo, object] : animations) {
                if (animInfo->animationId != animId) {
                    animId = animInfo->animationId;
                    currentAnim = animationManager.getAnimation<FramesSequence>(animId);
                }

                if (currentAnim) {
                    static_cast<Sprite*>(object)->setFrame(gSequenceAnimator.update2(nowTimeCount - animInfo->startTime, *currentAnim));
                }
            }
        }

        // _animations.clearAndReserve(); - moved to beginFrame(...)
    }

    void Scene::prepareDrawData(std::span<AttachmentObject*> objects, uint8_t frame) noexcept {
        // resize buffers
        auto spritesVertexesCount = _spritesVertexesCount;
        auto spritesIndicesCount = _spritesIndicesCount;

        auto meshTransformsInFrame = _meshTransformsInFrame;
        auto meshesDrawInFrame = _meshesDrawInFrame;
        auto skeletalTransformsInFrame = _skeletalTransformsInFrame;
        _tmpUniqueMeshes.reserve(kInitialmeshTransformsInFrameMax);

        auto addSkeletonToMap = [&](const Skeleton* skeleton)-> uint32_t {
            const uint32_t result = skeletalTransformsInFrame;
            const auto [it, success] = _meshTransformIndicesMap.emplace(skeleton, 
                        MeshTransformsIndices{
                            .transformIndex = kUnknownTransformIndex,
                            .skeletalTransformIndex = skeletalTransformsInFrame
                        });
            assert(success);
            skeletalTransformsInFrame += skeleton->skinMatricesCount();
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
                    const auto * skeleton = static_cast<const Mesh*>(object)->getSkeleton();
                    const bool isSkeletalMesh = skeleton != nullptr;

                    uint32_t skeletalTransformIndex = kUnknownTransformIndex;
                    if (isSkeletalMesh) { 
                        if (auto it = _meshTransformIndicesMap.find(skeleton); it != _meshTransformIndicesMap.end()) {
                            skeletalTransformIndex = it->second.skeletalTransformIndex;
                        } else {
                            skeletalTransformIndex = addSkeletonToMap(skeleton);
                        }
                    }
                    
                    const auto [it, success] = _meshTransformIndicesMap.emplace(object, 
                        MeshTransformsIndices{
                            .transformIndex = static_cast<uint32_t>(isSkeletalMesh ? kUnknownTransformIndex : meshTransformsInFrame),
                            .skeletalTransformIndex = skeletalTransformIndex
                        });
                    assert(success);

                    _tmpUniqueMeshes.emplace_back(it->first, it->second);
                
                    if (!isSkeletalMesh) {
                        ++meshTransformsInFrame;
                    }
                }
                ++meshesDrawInFrame; // every draw mesh has offset (for get correct matrices from meshTransforms)
            }
                break;
            case ctti::typeIndex<AttachmentObject, Sprite>():
                spritesVertexesCount += 4u;
                spritesIndicesCount += 6u;
                break;
            case ctti::typeIndex<AttachmentObject, Dummy>():
                break;
            }
        }

        {
            const auto size = static_cast<uint32_t>(spritesVertexesCount * kSpriteVertexSize);
            const auto currentSize = _spritesVertices->sizeInBytes();
            if (currentSize <= size) {
                _spritesVertices->resize(std::max(currentSize * 2u, size));
            }
        }

        {
            const auto size = static_cast<uint32_t>(spritesIndicesCount * sizeof(uint32_t));
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
            const auto size = static_cast<uint32_t>(meshesDrawInFrame * sizeof(uint32_t));
            const auto currentSize = _meshTransformIndices->sizeInBytes();
            if (currentSize <= size) {
                _meshTransformIndices->resize(std::max(currentSize * 2u, size));
            }
        }

        _spritesVertices->applySize(frame);
        _spritesIndices->applySize(frame);
        _meshTransforms->applySize(frame);
        _meshSkeletalTransforms->applySize(frame);
        _meshTransformIndices->applySize(frame);

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
    }

    void Scene::drawObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const mat4f & cameraTransform, AttachmentsIterator begin, AttachmentsIterator end, uint16_t typeIdx) {
        switch (typeIdx) {
        case ctti::typeIndex<AttachmentObject, Skeleton>(): // do not draw skeletons
            break;
        case ctti::typeIndex<AttachmentObject, Mesh>():
        {
            drawMeshes(commandList, frame, begin, end, cameraTransform);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Sprite>():
        {
            drawSprites(commandList, frame, begin, end, cameraTransform);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Dummy>():
        {
            drawDummies(begin, end, cameraTransform, 0xff'00'00'ffu);
        }
            break;
        }
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const mat4f & cameraTransform, std::span<AttachmentObject*> objects) {
        if (objects.empty()) {
            return;
        }

        prepareDrawData(objects, frame);

        auto currentIt = objects.begin();
        uint16_t typeIdx = (*currentIt)->typeIndex;
        for (auto it = currentIt + 1u; it != objects.end(); ++it) {
            const auto currenTypeIndex = (*it)->typeIndex;
            if (currenTypeIndex != typeIdx) {
                drawObjects(commandList, frame, cameraTransform, currentIt, it, typeIdx);

                currentIt = it;
                typeIdx = currenTypeIndex;
            }
        }

        drawObjects(commandList, frame, cameraTransform, currentIt, objects.end(), typeIdx);
    }

    void Scene::drawDummies(AttachmentsIterator begin, AttachmentsIterator end, const mat4f & cameraTransform, uint32_t color) {
        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _drawHelper.drawSphere(vec3f{0.0f}, 0.1f, color, 32u, cameraTransform * object->node->getWorldTransform());
        }
    }

    void Scene::drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const mat4f & cameraTransform) {
        auto currentIt = begin;
        auto it = currentIt;
        const auto & sprite = *static_cast<const Sprite*>(*it);
        uint32_t pipeline = sprite.getPipeline();
        auto image = sprite.getFrame().getImage();
        ++it;

        uint32_t batchVertexesCount = 4u;
        uint32_t batchIndicesCount = 6u;
        for (; it != end; ++it) {
            const auto & sprite = *static_cast<const Sprite*>(*it);
            const auto & currentImage = sprite.getFrame().getImage();
            const uint32_t currentPipeline = sprite.getPipeline();

            const bool completeBatch = currentPipeline != pipeline ||
                                       currentImage.get() != image.get();

            if (completeBatch) {
                // draw
                drawSpritesBatch(commandList, frame, currentIt, it, cameraTransform, batchVertexesCount, batchIndicesCount);
                // begin next batch
                image = currentImage;
                pipeline = currentPipeline;
                batchVertexesCount = 4u;
                batchIndicesCount = 6u;
                currentIt = it;
            } else {
                batchVertexesCount += 4u;
                batchIndicesCount += 6u;
            }
        }

        drawSpritesBatch(commandList, frame, currentIt, end, cameraTransform, batchVertexesCount, batchIndicesCount);
    }

    void Scene::drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const mat4f & cameraTransform, uint32_t batchVertexesCount, uint32_t batchIndicesCount) {

        const auto & sprite = *static_cast<const Sprite*>(*begin);
        const auto & pipeline = getPipeline(sprite.getPipeline());
        if (pipeline == nullptr) {
            return;
        }
        auto image = sprite.getFrame().getImage();
        const auto & imageExtent = image->getExtent();
        vec2f imageSize = {imageExtent.width, imageExtent.height};

        // const auto vSize = static_cast<uint32_t>((_spritesVertexesCount + batchVertexesCount) * kSpriteVertexSize);
        // const auto currentVSize = _spritesVertices->sizeInBytes();
        // if (currentVSize <= vSize) {
        //     _spritesVertices->resize(std::max(currentVSize * 2u, vSize));
        // }

        // const auto iSize = static_cast<uint32_t>((_spritesIndicesCount + batchIndicesCount) * sizeof(uint32_t));
        // const auto currentISize = _spritesIndices->sizeInBytes();
        // if (currentISize <= iSize) {
        //     _spritesIndices->resize(std::max(currentISize * 2u, iSize));
        // }

        auto * vMem = _spritesVertices->memory2<SpriteVertex>() + _spritesVertexesCount;
        auto * iMem = _spritesIndices->memory2<uint32_t>() + _spritesIndicesCount;

        uint32_t i = 0u, j = 0u;
        for (auto it = begin; it != end; ++it) {
            const auto & sprite = *static_cast<const Sprite*>(*it);
            const auto & frame2D = sprite.getFrame().getFrame();
            const auto & model = sprite.node->getWorldTransform();

            const vec2f corner0 = {0.0f, 0.0f};
            const vec2f corner1 = {(frame2D.uvMax[0u] - frame2D.uvMin[0u]) * imageSize.x, 
                                   (frame2D.uvMax[1u] - frame2D.uvMin[1u]) * imageSize.y};

            // fill vertexes and indices
            // vertexes
            using namespace graphics;
            auto v = model * vec4f(corner0.x, corner0.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 0u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMin[0u], frame2D.uvMax[1u])
            };
            v = model * vec4f(corner1.x, corner0.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 1u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMax[0u], frame2D.uvMax[1u])
            };
            v = model * vec4f(corner0.x, corner1.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 2u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMin[0u], frame2D.uvMin[1u])
            };
            v = model * vec4f(corner1.x, corner1.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 3u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMax[0u], frame2D.uvMin[1u])
            };
            
            // indices
            *(iMem + j + 0u) = _spritesVertexesCount + i + 0u;
            *(iMem + j + 1u) = _spritesVertexesCount + i + 1u;
            *(iMem + j + 2u) = _spritesVertexesCount + i + 2u;

            *(iMem + j + 3u) = _spritesVertexesCount + i + 2u;
            *(iMem + j + 4u) = _spritesVertexesCount + i + 1u;
            *(iMem + j + 5u) = _spritesVertexesCount + i + 3u;

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
            commandList->bindImage(0u, image, _linearSampler);
            commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(mat4f), &cameraTransform);

            commandList->drawIndexed(batchIndicesCount, 1u, _spritesIndicesCount);
        }
        _spritesVertexesCount += batchVertexesCount;
        _spritesIndicesCount += batchIndicesCount;
    }

    void Scene::drawMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, const mat4f & cameraTransform) {
        auto currentIt = begin;
        auto it = currentIt;
        const auto & mesh = *static_cast<const Mesh*>(*it);
        auto pipelineIdx = mesh.getPipeline();
        auto geometryDataIndex = mesh.geometryDataIndex();
        bool skeletal = mesh.getSkeleton() != nullptr;
        auto dataIndex = mesh.dataIndex();
        auto materialIndex = mesh.materialIndex();

        uint32_t i = 0u;
        auto * memory = _meshTransformIndices->memory2<uint32_t>() + _meshesDrawInFrame;
        if (skeletal) {
            *(memory + i) = _meshTransformIndicesMap[*it].skeletalTransformIndex;
        } else {
            *(memory + i) = _meshTransformIndicesMap[*it].transformIndex;
        }
        ++it;
        ++i;

        struct Uniform {
            mat4f cameraTransform;
            vec4f albedo;
        };

        Uniform uniform {
            .cameraTransform = cameraTransform, 
            .albedo = vec4f{1.0f}
        };

        auto applyMaterial = [this, &commandList, &uniform](uint16_t materialIndex) noexcept {
            const auto & material = _graphicsService.getMaterialManager().getMaterial(materialIndex);
            const auto & textures = material.textures;

            const auto & image = textures[static_cast<uint8_t>(graphics::PbrTextureId::eBaseColor)];
            assert(image != nullptr); // check textureManager default resources generated!

            // textures change more often than storage buffers - setId #1u used
            commandList->bindImage(0u, image, _linearSampler, 1u); 
            uniform.albedo = material.baseColorFactor;
        };

        uint32_t count = 1u;

        const auto & pipeline = getPipeline(pipelineIdx);
        assert(pipeline != nullptr);
        commandList->bindPipeline(pipeline);
        applyMaterial(materialIndex);

        if (skeletal) {
            commandList->bindBuffer(0u, (*_meshSkeletalTransforms)(frame), 0u, 0u);
        } else {
            commandList->bindBuffer(0u, (*_meshTransforms)(frame), 0u, 0u);
        }

        commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);

        for (; it != end; ++it, ++i) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            const auto currentPipelineIdx = mesh2.getPipeline();
            const auto currentGeometryDataIndex = mesh2.geometryDataIndex();
            const bool currentSkeletal = mesh2.getSkeleton() != nullptr;
            const auto currenDataIndex = mesh2.dataIndex();
            const auto currenMaterialIndex = mesh2.materialIndex();

            if (currentSkeletal) {
                *(memory + i) = _meshTransformIndicesMap[*it].skeletalTransformIndex;
            } else {
                *(memory + i) = _meshTransformIndicesMap[*it].transformIndex;
            }

            const bool pipelineChanged = pipelineIdx != currentPipelineIdx;
            const bool materialChanged = materialIndex != currenMaterialIndex;

            bool completePack = pipelineChanged ||
                                materialChanged ||
                                currentSkeletal != skeletal ||
                                currenDataIndex != dataIndex ||
                                currentGeometryDataIndex != geometryDataIndex;
                 
            if (completePack) {
                // draw
                drawSameMeshes(commandList, frame, currentIt, it, cameraTransform, count, skeletal);

                if (pipelineChanged) {
                    const auto & pipeline = getPipeline(currentPipelineIdx);
                    assert(pipeline != nullptr);
                    commandList->bindPipeline(pipeline);
                }

                if (pipelineChanged || materialChanged) {
                    applyMaterial(currenMaterialIndex);
                    commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);
                }

                if (currentSkeletal != skeletal) {
                    if (currentSkeletal) {
                        commandList->bindBuffer(0u, (*_meshSkeletalTransforms)(frame), 0u, 0u);
                    } else {
                        commandList->bindBuffer(0u, (*_meshTransforms)(frame), 0u, 0u);
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

        drawSameMeshes(commandList, frame, currentIt, end, cameraTransform, count, skeletal);

        _meshTransformIndices->modify(sizeof(uint32_t) * i);
    }

    void Scene::drawSameMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const mat4f & cameraTransform, uint32_t count, bool skeletal) {
            
        {
            const auto & mesh = *static_cast<const Mesh*>(*begin);

            uint32_t binding_v = 0u;
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::ePosition).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eNormal).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eUv0).buffer, 0u);

            if (skeletal) { // skeletalMesh
                // skinning in vertex shader impl (want to get skinning in compute too)
                commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eJoints).buffer, 0u);
                commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eWeights).buffer, 0u);
            }

            commandList->bindVertexBuffer(binding_v++, (*_meshTransformIndices)(frame), sizeof(uint32_t) * _meshesDrawInFrame);
            commandList->bindIndexBuffer(mesh.getIndices().buffer, 0u, rhi::IndexType::eU32);

            const auto & geometry = mesh.geometry();
            commandList->drawIndexed(geometry.indexCount, count, geometry.firstIndex);
        }

        _meshesDrawInFrame += count;
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
                auto const minCount = std::max(_meshTransformsInFrame, kInitialmeshTransformsInFrameMax);
                if (minCount * 2u < _lastFramesUsedMeshsTransforms) {
                    _meshTransforms->resize(minCount * kSsboTransformSize);
                    _tmpUniqueMeshes.resize(minCount);
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
                auto const minCount = std::max(_meshesDrawInFrame, kInitialmeshTransformsInFrameMax);
                if (minCount * 2u < _lastFramesUsedMeshesDraw) {
                    _meshTransformIndices->resize(minCount * sizeof(uint32_t));
                }
                _lastFramesUsedMeshesDraw = _meshesDrawInFrame;
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
                _skeletalTransformsInFrame > _lastFramesUsedSkeletalTransforms
            ) {
                _lastFramesUsedSpritesVertexCount = _spritesVertexesCount;
                _lastFramesUsedSpritesIndicesCount = _spritesIndicesCount;
                _lastFramesUsedMeshsTransforms = _meshTransformsInFrame;
                _lastFramesUsedMeshesDraw = _meshesDrawInFrame;
                _lastFramesUsedSkeletalTransforms = _skeletalTransformsInFrame;
                _tryReduceUsedMemoryTime = now;
            }
        }
    }

    void Scene::beginFrame(std::chrono::time_point<std::chrono::steady_clock> now) noexcept {
        reduceUsedMemory(now);
        _meshTransformIndicesMap.clear();
        _tmpUniqueMeshes.clear();
        _meshTransformIndicesMap.reserve(_meshTransformsInFrame);
    
        _spritesVertexesCount = 0u;
        _spritesIndicesCount = 0u;
        _meshTransformsInFrame = 0u;
        _meshesDrawInFrame = 0u;
        _skeletalTransformsInFrame = 0u;

        for (auto * taskW8 : _calculationWaiterPtrs) {
            taskW8->request_wait();
        }
        _calculationWaiterPtrs.clear();
        _animations.clearAndReserve();
    }

    void Scene::endFrame(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame) {
        performCommits(transferCommandList, frame);
    }

    uint32_t Scene::calculateTargetStagingSize() const noexcept { 
        constexpr auto bufferTargetSize = [](const graphics::InflightBuffer & buffer) noexcept {
            const auto size = buffer.sizeInBytes();
            return buffer.isHostVisible() ? 0u : (size < rhi::runtime::command_limits::kMaxMemoryBytesToCopyCommand ? 0u : size);
        };

        return bufferTargetSize(*_spritesVertices) + bufferTargetSize(*_spritesIndices) + 
               bufferTargetSize(*_meshTransforms) + bufferTargetSize(*_meshSkeletalTransforms) + bufferTargetSize(*_meshTransformIndices);
    }

    void Scene::performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        _spritesVertices->commit(*commandList, frame);
        _spritesIndices->commit(*commandList, frame);
        _meshTransforms->commit(*commandList, frame);
        _meshSkeletalTransforms->commit(*commandList, frame);
        _meshTransformIndices->commit(*commandList, frame);
    }
}