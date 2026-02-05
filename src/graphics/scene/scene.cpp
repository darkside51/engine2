#include "scene.h"
#include "animation.h"
#include "animation_manager.h"
#include "animation/texture/frame_sequence_animation.h"
#include "objects/sprite.h"
#include "objects/mesh.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/utils/vertex_processing.h"

#include <algorithm>
#include <array>

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
    constexpr uint32_t kDefaultSkeletoneBonesCount = 128;
    constexpr uint32_t kSsboSkeletalTransformSize = sizeof(SsboTransform) * kDefaultSkeletoneBonesCount;

    FramesSequenceAnimator gSequenceAnimator{1.0f / 30.0f};

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
        case ctti::typeIndex<AttachmentObject, Dummy>(): [[fallthrough]];
        case ctti::typeIndex<AttachmentObject, Skeleton>():
            node.setAttachment(&object);
            return;
        case ctti::typeIndex<AttachmentObject, Sprite>():
            static_cast<Sprite &>(object).attachToNode(&node);
            return;
        case ctti::typeIndex<AttachmentObject, Mesh>():
            static_cast<Mesh &>(object).attachToNode(&node);
            return;
        }
    }

    Scene::Scene(debug::DebugDraw * debugDraw) : _rootNode(createNode()), _debugDraw(debugDraw) {
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

        _sequences.clear();

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

        return it->second->destroyObject(object);
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
        _linearSampler.info.addressModeU = rhi::SamplerAddressMode::eClamp_To_Edge;
        _linearSampler.info.addressModeV = rhi::SamplerAddressMode::eClamp_To_Edge;
        _linearSampler.info.addressModeW = rhi::SamplerAddressMode::eClamp_To_Edge;
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
            api, device, verticesInfo, sizeof(uint16_t) * kInitialmeshTransformsInFrameMax
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
        case ctti::typeIndex<AttachmentObject, Dummy>():
            return static_cast<const Dummy &>(a).cmp(static_cast<const Dummy &>(b));
        case ctti::typeIndex<AttachmentObject, Sprite>():
            return static_cast<const Sprite &>(a).cmp(static_cast<const Sprite &>(b));
        case ctti::typeIndex<AttachmentObject, Mesh>():
            return static_cast<const Mesh &>(a).cmp(static_cast<const Mesh &>(b));
        case ctti::typeIndex<AttachmentObject, Skeleton>():
            return static_cast<const Skeleton &>(a).cmp(static_cast<const Skeleton &>(b));
        }

        return false;
    }

    void Scene::update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, std::span<AttachmentObject*> objects, 
        const AnimationManager & animationManager) {
        if (objects.empty()) {
            return;
        }

        // static thread_local std::vector<animation::AnimatedObject> sequences; // todo: remove static
        
        for (auto * object : objects) {
            const auto * animInfo = object->animInfo;
            while (animInfo) {
                if (animInfo->info.weight > 0.0f) {
                    switch (animInfo->info.animationType) {
                    case ctti::typeIndex<Animation, FramesSequence>() : 
                    {
                        _sequences.emplace_back(&animInfo->info, object);
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

        if (!_sequences.empty()) { // sequences
            std::sort(_sequences.begin(), _sequences.end(), [](const auto & seq0, const auto & seq1) {
                        return seq0.animInfo->animationId < seq1.animInfo->animationId;
                    });

            auto animId = kUndefinedAnimId;
            const FramesSequence * currentAnim = nullptr;
            
            for (const auto & [animInfo, object] : _sequences) {
                if (animInfo->animationId != animId) {
                    animId = animInfo->animationId;
                    currentAnim = animationManager.getAnimation<FramesSequence>(animId);
                }

                if (currentAnim) {
                    static_cast<Sprite*>(object)->setFrame(gSequenceAnimator.update2(nowTimeCount - animInfo->startTime, *currentAnim));
                }
            }
        }

        auto const seqSize = _sequences.size();
        _sequences.clear();
        _sequences.reserve(seqSize);

        // todo: other types!
    }

    void Scene::prepareDrawData(std::span<AttachmentObject*> objects, uint8_t frame) noexcept {
        // resize buffers
        auto spritesVertexesCount = _spritesVertexesCount;
        auto spritesIndicesCount = _spritesIndicesCount;

        auto meshTransformsInFrame = _meshTransformsInFrame;
        auto meshesDrawInFrame = _meshesDrawInFrame;
        auto skeletalTransformsInFrame = _skeletalTransformsInFrame;
        _tmpUniqueMeshes.reserve(kInitialmeshTransformsInFrameMax);

        for (auto const * object : objects) {
            switch (object->typeIndex) {
            case ctti::typeIndex<AttachmentObject, Dummy>(): [[fallthrough]];
            case ctti::typeIndex<AttachmentObject, Skeleton>():
                break;
            case ctti::typeIndex<AttachmentObject, Sprite>():
                spritesVertexesCount += 4u;
                spritesIndicesCount += 6u;
                break;
            case ctti::typeIndex<AttachmentObject, Mesh>():
            {
                if (!_meshTransformIndicesMap.contains(object)) {
                    const auto * skeleton = static_cast<const Mesh*>(object)->getSkeleton();
                    const bool isSkeletalMesh = skeleton != nullptr;
                    const auto [it, success] = _meshTransformIndicesMap.emplace(object, 
                        MeshTransformsIndices{
                            .transformIndex = static_cast<uint16_t>(isSkeletalMesh ? 0xff'ffu : meshTransformsInFrame),
                            .skeletalTransformIndex = static_cast<uint16_t>(isSkeletalMesh ? skeletalTransformsInFrame : 0xff'ffu)
                        });
                    assert(success);

                    _tmpUniqueMeshes.emplace_back(it->first, it->second);
                
                    if (isSkeletalMesh) {
                        skeletalTransformsInFrame += skeleton->skinMatricesCount();
                    } else {
                        ++meshTransformsInFrame;
                    }
                }
                ++meshesDrawInFrame; // every draw mesh has offset (for get correct matrices from meshTransforms)
            }
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
            const auto size = static_cast<uint32_t>(meshesDrawInFrame * sizeof(uint16_t));
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
                if (idx.transformIndex != 0xff'ffu) {
                    memcpy(
                        (mem + idx.transformIndex), 
                        &glm::transpose(object->node->getWorldTransform())[0u][0u], 
                        sizeof(SsboTransform)
                    );

                    // const auto tr = glm::transpose(object->node->getWorldTransform());
                    // *(mem + idx) = {
                    //     tr[0u],
                    //     tr[1u],
                    //     tr[2u]
                    // };
                    // *(mem + idx) = object->node->getWorldTransform();
                }

                if (idx.skeletalTransformIndex != 0xff'ffu) {
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
        case ctti::typeIndex<AttachmentObject, Dummy>():
        {
            drawDummies(begin, end, cameraTransform, 0xff'00'00'ffu);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Sprite>():
        {
            drawSprites(commandList, frame, begin, end, cameraTransform);
        }
            break;
        case ctti::typeIndex<AttachmentObject, Mesh>():
        {
            drawMeshes(commandList, frame, begin, end, cameraTransform);
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

        performCommits(transferCommandList, frame);
    }

    void Scene::drawDummies(AttachmentsIterator begin, AttachmentsIterator end, const mat4f & cameraTransform, uint32_t color) {
        if (_debugDraw == nullptr) {
            return;
        }

        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _debugDraw->drawSphere(vec3f{0.0f}, 0.1f, color, 32u, cameraTransform * object->node->getWorldTransform());
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
        uint32_t pipeline = mesh.getPipeline();
        const auto * meshData = &mesh.meshData();
        ++it;

        for (; it != end; ++it) {
            const auto & mesh2 = *static_cast<const Mesh*>(*it);
            const uint32_t currentPipeline = mesh2.getPipeline();
            const auto * currentMeshData = &mesh2.meshData();

            const bool completePack = currentPipeline != pipeline ||
                                      meshData != currentMeshData;

            if (completePack) {
                // draw
                drawSameMeshes(commandList, frame, currentIt, it, cameraTransform);
                // begin next pack
                meshData = currentMeshData;
                pipeline = currentPipeline;
                currentIt = it;
            }
        }

        drawSameMeshes(commandList, frame, currentIt, end, cameraTransform);
    }

    void Scene::drawSameMeshes(rhi::runtime::CommandListPtr & commandList, uint8_t frame, AttachmentsIterator begin, AttachmentsIterator end, 
        const mat4f & cameraTransform) {
        const auto & mesh = *static_cast<const Mesh*>(*begin);
        const auto & pipeline = getPipeline(mesh.getPipeline());
        if (pipeline == nullptr) {
            return;
        }

        const bool isSkeletalMesh = mesh.getSkeleton() != nullptr;

        auto * mem2 = _meshTransformIndices->memory2<uint16_t>() + _meshesDrawInFrame;
        uint32_t drawObjectsCount = 0u;
        for (auto it = begin; it != end; ++it) {
            if (isSkeletalMesh) {
                *(mem2 + drawObjectsCount) = _meshTransformIndicesMap[*it].skeletalTransformIndex;
            } else {
                *(mem2 + drawObjectsCount) = _meshTransformIndicesMap[*it].transformIndex;
            }
            ++drawObjectsCount;
        }
        assert(drawObjectsCount < std::numeric_limits<uint16_t>::max()); // need to use 32bits _meshTransformIndices instead of 16bits

        _meshTransformIndices->modify();

        struct Uniform {
            mat4f cameraTransform;
        };

       const Uniform uniform {.cameraTransform = cameraTransform};

        if (auto & transforms = (*_meshTransforms)(frame)) {
            commandList->bindPipeline(pipeline);

            uint32_t binding_v = 0u;
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::ePosition).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eNormal).buffer, 0u);
            commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eUv0).buffer, 0u);

            if (isSkeletalMesh) {
                // skinning in vertex shader impl (want to get skinning in compute too)
                commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eJoints).buffer, 0u);
                commandList->bindVertexBuffer(binding_v++, mesh.getAttribute(mesh::Semantic::eWeights).buffer, 0u);
                commandList->bindVertexBuffer(binding_v++, (*_meshTransformIndices)(frame), sizeof(uint16_t) * _meshesDrawInFrame);
                commandList->bindBuffer(0u, (*_meshSkeletalTransforms)(frame), 0u, 0u);
            } else {
                commandList->bindVertexBuffer(binding_v++, (*_meshTransformIndices)(frame), sizeof(uint16_t) * _meshesDrawInFrame);
                commandList->bindBuffer(0u, (*_meshTransforms)(frame), 0u, 0u);
            }

            commandList->bindIndexBuffer(mesh.getIndices().buffer, 0u, rhi::IndexType::eU32);

            commandList->pushConstants(rhi::shaders::Stages::eVertex, 0u, sizeof(Uniform), &uniform);

            const auto & geometry = mesh.geometry();
            for (const auto & p : geometry.primitives) {
                commandList->drawIndexed(p.indexCount, drawObjectsCount, p.firstIndex);
            }
        }

        _meshesDrawInFrame += drawObjectsCount;
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
                    _meshTransformIndices->resize(minCount * sizeof(uint16_t));
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

    void Scene::newFrame(std::chrono::time_point<std::chrono::steady_clock> now) noexcept {
        reduceUsedMemory(now);
        _meshTransformIndicesMap.clear();
        _tmpUniqueMeshes.clear();
        _meshTransformIndicesMap.reserve(_meshTransformsInFrame);
    
        _spritesVertexesCount = 0u;
        _spritesIndicesCount = 0u;
        _meshTransformsInFrame = 0u;
        _meshesDrawInFrame = 0u;
        _skeletalTransformsInFrame = 0u;
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