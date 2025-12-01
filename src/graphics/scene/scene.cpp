#include "scene.h"
#include "bounds.h"
#include "frustum.h"
#include "sprite/sprite.h"

#include "graphics/render/rhi/common/command_list.h"

#include <algorithm>

namespace engine::scene {

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

    Scene::Scene(debug::DebugDraw * debugDraw) : _rootNode(createNode()), _debugDraw(debugDraw) {
        registerTypes();
    }

    Scene::~Scene() {
        destroyNode(_rootNode);

        // for (auto & [_, objects] : _objectCollections) {
        // }
    }

    Node * Scene::createNode() noexcept {
        return _nodeAllocator.create<engine::scene::Node>(false);
    }

    Node * Scene::createFinalNode() noexcept {
        return _nodeAllocator.create<engine::scene::Node>(true);
    }

    void Scene::destroyNode(Node * node) noexcept {
        if (node->removeFromParent() || node == _rootNode) {
            if (Node * n = node->isIntermediate() ? node->data.link.firstChild : nullptr) {
                while (n->next) {
                    auto * n1 = n->next;
                    destroyNode(n);
                    n = n1;
                }
                destroyNode(n);
            }
            if (node->isFinal()) {
                if (auto * object = node->getObject()) {
                    destroyObject(object);
                }
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

        return it->second->destroyObject(object);
    }

    void Scene::newFrame() noexcept {
        _spritesVertexesCount = 0u;
        _spritesIndicesCount = 0u;
    }

    IntrusiveList<Object> Scene::recalculate(const mat4f & cameraTransform) noexcept {
        const Frustum frustum(cameraTransform);
        IntrusiveList<Object> objects;

        recalculateHierarchy(_rootNode, [this, &frustum, &cameraTransform, &objects](Node * n) noexcept { 
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

            if (n->isFinal()) {
                if (auto * object = n->getObject()) {
                    object->matrix = &n->getWorldTransform();
                    objects.emplace(object);
                }
            }

            if (_debugDraw) {
                const auto transform = cameraTransform * worldTransform;
                _debugDraw->drawGizmo(1.0f, transform);
                _debugDraw->drawBounds(bounds, 0xff00ffffu, transform);
            }
            return true;
        });

        return objects;
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
        //create vertex and index buffers
        constexpr uint32_t kInitialVerticesCountMax = 1024u;
        constexpr uint32_t kInitialIndicesCountMax = 1024u;

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

    bool Scene::compareObjects(const Object & a, const Object & b) const noexcept {
        switch(a.typeIndex) {
        case ctti::typeIndex<Object, EmptyObject>():
            return static_cast<const EmptyObject &>(a).cmp(static_cast<const EmptyObject &>(b));
        case ctti::typeIndex<Object, Sprite>():
            return static_cast<const Sprite &>(a).cmp(static_cast<const Sprite &>(b));
        default:
            return false;
        }
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
            return compareObjects(a, b);
        });

        auto currentIt = objects.begin();
        uint16_t typeIdx = currentIt->typeIndex;
        for (auto it = currentIt; it != objects.end(); ++it) {
            bool const last = it.node()->next == nullptr;
            if (it->typeIndex != typeIdx || last) {
                switch (typeIdx) {
                case ctti::typeIndex<Object, EmptyObject>():
                {
                    drawEmptyObjects(currentIt, (last ? objects.end(): it), cameraTransform, 0xff'00'00'ffu);
                }
                    break;
                case ctti::typeIndex<Object, Sprite>():
                {
                    drawSprites(commandList, frame, currentIt, (last ? objects.end(): it), cameraTransform);
                }
                    break;
                default:
                    break;
                }

                currentIt = it;
                typeIdx = it->typeIndex;
            }
        }

        performCommits(transferCommandList, frame);
    }

    void Scene::drawEmptyObjects(Object::Iterator begin, Object::Iterator end, const mat4f & cameraTransform, uint32_t color) {
        if (_debugDraw == nullptr) {
            return;
        }

        for (auto it = begin; it != end; ++it) {
            const auto & object = *it;
            _debugDraw->drawSphere(vec3f{0.0f}, static_cast<const EmptyObject&>(object).size, color, 
                32u, cameraTransform * (*object.matrix));
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

            const bool completeBatch = it.node()->next == end.node() ||
                                 currentPipeline != pipeline ||
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

        const auto vSize = static_cast<uint32_t>((_spritesVertexesCount + batchVertexesCount) * kSpriteVertexSize);
        const auto currentVSize = _spritesVertices->sizeInBytes();
        if (currentVSize <= vSize) {
            _spritesVertices->resize(std::max(currentVSize * 2u, vSize));
        }

        const auto iSize = static_cast<uint32_t>((_spritesIndicesCount + batchIndicesCount) * sizeof(uint32_t));
        const auto currentISize = _spritesIndices->sizeInBytes();
        if (currentISize <= iSize) {
            _spritesIndices->resize(std::max(currentISize * 2u, iSize));
        }

        auto * vMem = _spritesVertices->memory2<SpriteVertex>() + _spritesVertexesCount;
        auto * iMem = _spritesIndices->memory2<uint32_t>() + _spritesIndicesCount;

        uint32_t i = 0u, j = 0u;
        for (auto it = begin; it != end; ++it) {
            const auto & sprite = static_cast<const Sprite&>(*it);
            const auto & frame2D = sprite.getFrame().getFrame();
            const auto mvp = cameraTransform * (*sprite.matrix);

            const vec2f corner0 = {frame2D.uvMin[0u] * imageSize.x, frame2D.uvMin[1u] * imageSize.y};
            const vec2f corner1 = {frame2D.uvMax[0u] * imageSize.x, frame2D.uvMax[1u] * imageSize.y};

            // fill vertexes and indices
            // vertexes
            *(vMem + i + 0u) = {
                .position = mvp * vec4f(corner0.x, corner0.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMin[0u], frame2D.uvMin[1u]}
            };
            *(vMem + i + 1u) = {
                .position = mvp * vec4f(corner1.x, corner0.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMax[0u], frame2D.uvMin[1u]}
            };
            *(vMem + i + 2u) = {
                .position = mvp * vec4f(corner0.x, corner1.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMin[0u], frame2D.uvMax[1u]}
            };
            *(vMem + i + 3u) = {
                .position = mvp * vec4f(corner1.x, corner1.y, 0.0f, 1.0f),
                .uv = {frame2D.uvMax[0u], frame2D.uvMax[1u]}
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

        commandList->bindPipeline(pipeline);
        commandList->bindVertexBuffer(0u, (*_spritesVertices)(frame), 0u);
        commandList->bindIndexBuffer((*_spritesIndices)(frame), 0u, rhi::IndexType::eU32);
        commandList->bindImage(0u, image, rhi::SamplerValue{}); // todo: sampler!

        commandList->drawIndexed(batchIndicesCount, 1u, _spritesIndicesCount);
        _spritesVertexesCount += batchVertexesCount;
        _spritesIndicesCount += batchIndicesCount;
    }


    void Scene::performCommits(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        // const bool useStagingBuffer = !_spritesVertices->isHostVisible() || !_spritesIndices->isHostVisible();
        // todo: 

        if (_spritesVertices) {
            _spritesVertices->commit(*commandList, frame);
        }
        if (_spritesIndices) {
            _spritesIndices->commit(*commandList, frame);
        }
    }
}