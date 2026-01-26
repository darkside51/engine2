#include "scene.h"
#include "animation.h"
#include "animation_manager.h"
#include "animation/texture/frame_sequence_animation.h"
#include "sprite/sprite.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/utils/vertex_processing.h"

#include <algorithm>
#include <array>

#include <unordered_map>

namespace engine::scene {

    constexpr float kTryReduceUsedMemoryTimePeriodSeconds = 5.0f;
    constexpr uint32_t kInitialVerticesCountMax = 1024u;
    constexpr uint32_t kInitialIndicesCountMax = 1024u;
    constexpr uint32_t kMinStagingSize = 100u * 1024u /*100 Kb*/;
    constexpr uint32_t kStaginBufferEnlargeSizeMult = 2u;

    constexpr uint16_t kUndefinedAnimId = 0xff'ffu;

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

    void attachObjectToNode(Object & object, Node & node) noexcept {
        switch(object.typeIndex) {
        case ctti::typeIndex<Object, Dummy>():
            node.setObject(&object);
            return;
        case ctti::typeIndex<Object, Sprite>():
            static_cast<Sprite &>(object).attachToNode(&node);
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

            if (auto * object = node->getObject()) {
                destroyObject(object);
            }

            _nodeAllocator.destroy(node);
        }
    }

    bool Scene::destroyObject(Object * object) noexcept {
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
            return nullptr;
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

        //
    }

    bool Scene::compareObjectsForDraw(const Object & a, const Object & b) noexcept {
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
        case ctti::typeIndex<Object, Dummy>():
            return static_cast<const Dummy &>(a).cmp(static_cast<const Dummy &>(b));
        case ctti::typeIndex<Object, Sprite>():
            return static_cast<const Sprite &>(a).cmp(static_cast<const Sprite &>(b));
        default:
            return false;
        }
    }

    void Scene::update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, std::span<Object*> objects, 
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
                    static_cast<Sprite*>(object)->setFrame(gSequenceAnimator.update(nowDuration, animInfo->startTime, *currentAnim));
                }
            }
        }

        auto const seqSize = _sequences.size();
        _sequences.clear();
        _sequences.reserve(seqSize);

        // todo: other types!
    }

    void Scene::prepareDrawData(std::span<Object*> objects, uint8_t frame) noexcept {
        // resize buffers
        auto spritesVertexesCount = _spritesVertexesCount;
        auto spritesIndicesCount = _spritesIndicesCount;
        for (auto const * object : objects) {
            switch (object->typeIndex) {
            case ctti::typeIndex<Object, Sprite>():
                spritesVertexesCount += 4u;
                spritesIndicesCount += 6u;
                break;
            default:
                break;
            }
        }

        const auto vSize = static_cast<uint32_t>((spritesVertexesCount) * kSpriteVertexSize);
        const auto currentVSize = _spritesVertices->sizeInBytes();
        bool spiteVtxChanged = false;
        if (currentVSize <= vSize) {
            _spritesVertices->resize(std::max(currentVSize * 2u, vSize));
        }

        const auto iSize = static_cast<uint32_t>((spritesIndicesCount) * sizeof(uint32_t));
        const auto currentISize = _spritesIndices->sizeInBytes();
        bool spiteIdxChanged = false;
        if (currentISize <= iSize) {
            _spritesIndices->resize(std::max(currentISize * 2u, iSize));
        }

        _spritesVertices->applySize(frame);
        _spritesIndices->applySize(frame);

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
        }
    }

    void Scene::drawObjects(rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
        const mat4f & cameraTransform, ObjectsIterator begin, ObjectsIterator end, uint16_t typeIdx) {
        switch (typeIdx) {
        case ctti::typeIndex<Object, Dummy>():
        {
            drawDummies(begin, end, cameraTransform, 0xff'00'00'ffu);
        }
            break;
        case ctti::typeIndex<Object, Sprite>():
        {
            drawSprites(commandList, frame, begin, end, cameraTransform);
        }
            break;
        default:
            break;
        }
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const mat4f & cameraTransform, std::span<Object*> objects) {
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

    void Scene::drawDummies(ObjectsIterator begin, ObjectsIterator end, const mat4f & cameraTransform, uint32_t color) {
        if (_debugDraw == nullptr) {
            return;
        }

        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _debugDraw->drawSphere(vec3f{0.0f}, 0.1f, color, 32u, cameraTransform * object->node->getWorldTransform());
        }
    }

    void Scene::drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, ObjectsIterator begin, ObjectsIterator end, 
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

    void Scene::drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, ObjectsIterator begin, ObjectsIterator end, 
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
            const auto mvp = cameraTransform * sprite.node->getWorldTransform();

            const vec2f corner0 = {0.0f, 0.0f};
            const vec2f corner1 = {(frame2D.uvMax[0u] - frame2D.uvMin[0u]) * imageSize.x, 
                                   (frame2D.uvMax[1u] - frame2D.uvMin[1u]) * imageSize.y};

            // fill vertexes and indices
            // vertexes
            using namespace graphics;
            auto v = mvp * vec4f(corner0.x, corner0.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 0u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMin[0u], frame2D.uvMax[1u])
            };
            v = mvp * vec4f(corner1.x, corner0.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 1u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMax[0u], frame2D.uvMax[1u])
            };
            v = mvp * vec4f(corner0.x, corner1.y, 0.0f, 1.0f);
            v /= v.w;
            *(vMem + i + 2u) = {
                .position = {v.x, v.y, v.z},
                .uv = to_vec2u16(frame2D.uvMin[0u], frame2D.uvMin[1u])
            };
            v = mvp * vec4f(corner1.x, corner1.y, 0.0f, 1.0f);
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

            commandList->drawIndexed(batchIndicesCount, 1u, _spritesIndicesCount);
        }
        _spritesVertexesCount += batchVertexesCount;
        _spritesIndicesCount += batchIndicesCount;
    }

    void Scene::reduceUsedMemory(std::chrono::time_point<std::chrono::steady_clock> now) noexcept {
        const std::chrono::duration<float> duration = now - _tryReduceUsedMemoryTime; // as default in seconds
		const auto elapsed = duration.count();

        if (elapsed >= kTryReduceUsedMemoryTimePeriodSeconds) {
            _tryReduceUsedMemoryTime = now;

            auto const vertexMinCount = std::max(_spritesVertexesCount, kInitialVerticesCountMax);
            if (vertexMinCount * 2u < _lastFramesUsedSpritesVertexCount) {
                _spritesVertices->resize(vertexMinCount * kSpriteVertexSize);
            }
            _lastFramesUsedSpritesVertexCount = _spritesVertexesCount;

            auto const indexMinCount = std::max(_spritesIndicesCount, kInitialIndicesCountMax);
            if (indexMinCount * 2u < _lastFramesUsedSpritesIndicesCount) {
                _spritesIndices->resize(indexMinCount * sizeof(uint32_t));
            }
            _lastFramesUsedSpritesIndicesCount = _spritesIndicesCount;

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
            if (_spritesVertexesCount > _lastFramesUsedSpritesVertexCount || _spritesIndicesCount > _lastFramesUsedSpritesIndicesCount) {
                _lastFramesUsedSpritesVertexCount = _spritesVertexesCount;
                _lastFramesUsedSpritesIndicesCount = _spritesIndicesCount;
                _tryReduceUsedMemoryTime = now;
            }
        }
    }

    void Scene::newFrame(std::chrono::time_point<std::chrono::steady_clock> now) noexcept {
        reduceUsedMemory(now);
        _spritesVertexesCount = 0u;
        _spritesIndicesCount = 0u;
    }

    uint32_t Scene::calculateTargetStagingSize() const noexcept {
        const bool useStagingBuffer = !_spritesVertices->isHostVisible() || !_spritesIndices->isHostVisible();
        if (useStagingBuffer) {
            return _spritesVertices->sizeInBytes() + _spritesIndices->sizeInBytes();
        }
        return 0u;
    }

    void Scene::performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        _spritesVertices->commit(*commandList, frame);
        _spritesIndices->commit(*commandList, frame);
    }
}