#include "scene.h"
#include "animation.h"
#include "bounds.h"
#include "frustum.h"
#include "animation/texture/frame_sequence_animation.h"
#include "sprite/sprite.h"

#include "graphics/render/rhi/common/command_list.h"

#include <algorithm>

#include <unordered_map>

namespace engine::scene {

    constexpr uint32_t kInitialVerticesCountMax = 1024u;
    constexpr uint32_t kInitialIndicesCountMax = 1024u;
    constexpr bool kDebugDrawBounds = true;

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
        vec4f position;
        vec2f uv;
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
        _spritesVertices = nullptr;
        _spritesIndices = nullptr;
        _staging = nullptr;
        _pipelines.clear();

        destroyNode(_rootNode);
    }

    Node * Scene::createNode() noexcept {
        return _nodeAllocator.create<Node>();
    }

    void Scene::destroyNode(Node * node) noexcept {
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
        }

        return it->second->destroyObject(object);
    }

    AnimationInfoChain * Scene::createAnimInfo(uint16_t animationId, uint16_t animationType, float startTime, float weight) {
        return _animationInfosAllocator.create<AnimationInfoChain>(startTime, weight, animationId, animationType);
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

    bool Scene::compareObjectsForDraw(const Object & a, const Object & b) const noexcept {
        switch(a.typeIndex) {
        case ctti::typeIndex<Object, Dummy>():
            return static_cast<const Dummy &>(a).cmp(static_cast<const Dummy &>(b));
        case ctti::typeIndex<Object, Sprite>():
            return static_cast<const Sprite &>(a).cmp(static_cast<const Sprite &>(b));
        default:
            return false;
        }
    }

    IntrusiveList<Object> Scene::makeRenderList(const mat4f & cameraTransform, Node * root) noexcept {
        const Frustum frustum(cameraTransform);
        IntrusiveList<Object> objects;

        recalculateHierarchy(root, [this, &frustum, &cameraTransform, &objects](Node * n) noexcept { 
            if (n->isHidden()) {
                return false;
            }

            const auto & bounds = n->bounds;
            const auto & worldTransform =  n->getWorldTransform();
            if (bounds.type != Bounds::Type::eNone) {
                if (!frustum.checkBounds(bounds.transform(worldTransform))) {
                    return false;
                }
            }

            if (auto * object = n->getObject()) {
                objects.emplace(object);
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

        return objects;
    }

    void Scene::update(std::chrono::time_point<std::chrono::steady_clock> now, float delta, IntrusiveList<Object> & objects, const Animation & tmpAnimTest) {
        if (objects.empty()) {
            return;
        }

        struct AnimatedObject {
            const AnimationInfo* animInfo = nullptr;
            Object * object = nullptr;
        };

        static thread_local std::vector<AnimatedObject> sequences; // todo: remove static
        sequences.clear();

        for (auto & object : objects) {
            const auto * animInfo = object.animInfo;
            while (animInfo) {
                if (animInfo->info.weight > 0.0f) {
                    switch (animInfo->info.animationType) {
                    case ctti::typeIndex<Animation, FramesSequence>() : 
                    {
                        sequences.emplace_back(&animInfo->info, &object);
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

        if (!sequences.empty()) { // sequences
            std::sort(sequences.begin(), sequences.end(), [](const auto & seq0, const auto & seq1) {
                        return seq0.animInfo->animationId < seq1.animInfo->animationId;
                    });

            auto animId = sequences[0u].animInfo->animationId;
            const FramesSequence * currentAnim = static_cast<const FramesSequence*>(&tmpAnimTest); // animationManager.getAnimation<FramesSequence>(animId);
            
            for (const auto & [animInfo, object] : sequences) {
                if (animInfo->animationId != animId) {
                    animId = animInfo->animationId;
                    currentAnim = static_cast<const FramesSequence*>(&tmpAnimTest);
                }

                if (currentAnim) {
                    static_cast<Sprite*>(object)->setFrame(gSequenceAnimator.update(nowDuration, animInfo->startTime, *currentAnim));
                }
            }
        }

        // todo: other types!
    }

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, rhi::runtime::CommandListPtr & transferCommandList, uint8_t frame, 
        const mat4f & cameraTransform, IntrusiveList<Object> & objects) {
        if (objects.empty()) {
            return;
        }

        objects.stable_sort([this](const Object & a, const Object & b) noexcept {
            // if (a.layer != b.layer) {
            //     return a.layer < b.layer;
            // }

            if (a.order != b.order) {
                return a.order < b.order;
            }

            if (a.typeIndex != b.typeIndex) {
                return a.typeIndex < b.typeIndex;
            }

            // todo special sorts for some types
            return compareObjectsForDraw(a, b);
        });

        const auto drawObjects = [this](rhi::runtime::CommandListPtr & commandList, uint8_t frame, 
            const mat4f & cameraTransform, Object::Iterator begin, Object::Iterator end, uint16_t typeIdx) noexcept {
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
        };

        auto currentIt = objects.begin();

        // resize buffers
        // todo: make as class function
        auto spritesVertexesCount = _spritesVertexesCount;
        auto spritesIndicesCount = _spritesIndicesCount;
        for (auto it = currentIt; it != objects.end(); ++it) {
            switch (it->typeIndex) {
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
        //

        uint16_t typeIdx = currentIt->typeIndex;
        for (auto it = currentIt; it != objects.end(); ++it) {
            if (it->typeIndex != typeIdx) {
                drawObjects(commandList, frame, cameraTransform, currentIt, it, typeIdx);

                currentIt = it;
                typeIdx = it->typeIndex;
            }
        }

        drawObjects(commandList, frame, cameraTransform, currentIt, objects.end(), typeIdx);

        performCommits(transferCommandList, frame);
    }

    void Scene::drawDummies(Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform, uint32_t color) {
        if (_debugDraw == nullptr) {
            return;
        }

        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _debugDraw->drawSphere(vec3f{0.0f}, 0.1f, color, 32u, cameraTransform * object.node->getWorldTransform());
        }
    }

    void Scene::drawSprites(rhi::runtime::CommandListPtr & commandList, uint8_t frame, Object::Iterator begin, Object::Iterator end, 
        const mat4f & cameraTransform) {
        auto currentIt = begin;
        auto it = currentIt;
        const auto & sprite = static_cast<const Sprite&>(*it);
        uint32_t pipeline = sprite.getPipeline();
        auto image = sprite.getFrame().getImage();
        ++it;

        uint32_t batchVertexesCount = 4u;
        uint32_t batchIndicesCount = 6u;
        for (; it != end; ++it) {
            const auto & sprite = static_cast<const Sprite&>(*it);
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

    void Scene::drawSpritesBatch(rhi::runtime::CommandListPtr & commandList, uint8_t frame, Object::Iterator begin, Object::Iterator end, 
        const mat4f & cameraTransform, uint32_t batchVertexesCount, uint32_t batchIndicesCount) {

        const auto & sprite = static_cast<const Sprite&>(*begin);
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
            const auto & sprite = static_cast<const Sprite&>(*it);
            const auto & frame2D = sprite.getFrame().getFrame();
            const auto mvp = cameraTransform * sprite.node->getWorldTransform();

            const vec2f corner0 = {0.0f, 0.0f};
            const vec2f corner1 = {(frame2D.uvMax[0u] - frame2D.uvMin[0u]) * imageSize.x, 
                                   (frame2D.uvMax[1u] - frame2D.uvMin[1u]) * imageSize.y};

            // fill vertexes and indices
            // vertexes
            *(vMem + i + 0u) = {
                .position = mvp * vec4f(corner0.x, corner0.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMin[0u], frame2D.uvMax[1u]}
            };
            *(vMem + i + 1u) = {
                .position = mvp * vec4f(corner1.x, corner0.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMax[0u], frame2D.uvMax[1u]}
            };
            *(vMem + i + 2u) = {
                .position = mvp * vec4f(corner0.x, corner1.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMin[0u], frame2D.uvMin[1u]}
            };
            *(vMem + i + 3u) = {
                .position = mvp * vec4f(corner1.x, corner1.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMax[0u], frame2D.uvMin[1u]}
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

        if (auto & vertexBuffer = (*_spritesVertices)(frame)) { // hmmm, must be after buffer commits!
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

    void Scene::newFrame() noexcept {
        auto const vertexMinCount = std::max(_spritesVertexesCount, kInitialVerticesCountMax);
        if (vertexMinCount * 2u < _lastFramesUsedSpritesVertexCount) {
            _spritesVertices->resize(vertexMinCount * kSpriteVertexSize);
        }

        auto const indexMinCount = std::max(_spritesIndicesCount, kInitialIndicesCountMax);
        if (indexMinCount * 2u < _lastFramesUsedSpritesIndicesCount) {
            _spritesIndices->resize(indexMinCount * sizeof(uint32_t));
        }

        _lastFramesUsedSpritesVertexCount = _spritesVertexesCount;
        _lastFramesUsedSpritesIndicesCount = _spritesIndicesCount;

        _spritesVertexesCount = 0u;
        _spritesIndicesCount = 0u;
    }

    void Scene::performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        const bool useStagingBuffer = !_spritesVertices->isHostVisible() || !_spritesIndices->isHostVisible();
        if (useStagingBuffer) {
            const auto allSize = _spritesVertices->sizeInBytes() + _spritesIndices->sizeInBytes();
            if (_staging == nullptr || allSize >= _staging->sizeInBytes()) {
                constexpr uint32_t kMinStagingSize = 100u * 1024u /*100 Kb*/;
                _staging = std::make_unique<graphics::InflightStagingBuffer>(*_api, *_device, std::max(allSize * 2u, kMinStagingSize), 
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
            }
        }

        if (_staging) {
            _staging->map(frame); // reset staging data into + mapped memory set frame

            _spritesVertices->setStagingBuffer(_staging.get());
            _spritesIndices->setStagingBuffer(_staging.get());
        }
        
        _spritesVertices->commit(*commandList, frame);
        _spritesIndices->commit(*commandList, frame);
    }
}