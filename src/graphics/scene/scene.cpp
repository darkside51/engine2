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

    void Scene::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform, IntrusiveList<Object> & objects) {
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

        // const auto vSize = static_cast<uint32_t>((_verticesCount + linesVtxs.size()) * kVertexSize);
        // if (_vertices->sizeInBytes() <= vSize) {
        //     _vertices->resize(std::max(_vertices->sizeInBytes() * 2u, vSize));
        // }

        // const auto lineParts = linesVtxs.size() - 1u;
        // const auto newIndicesCount = lineParts * 2u;
        // const auto iSize = static_cast<uint32_t>((_lineIndicesCount + newIndicesCount) * sizeof(uint32_t));
        // if (_linesIndices->sizeInBytes() <= iSize) {
        //     _linesIndices->resize(std::max(_linesIndices->sizeInBytes() * 2u, iSize));
        // }

        uint32_t indexCount = 0u;
        for (auto it = begin; it != end; ++it) {
            const auto & sprite = static_cast<const Sprite&>(*it);
            const auto & frame2D = sprite.getFrame().getFrame();
            const auto mvp = cameraTransform * (*sprite.matrix);

            const vec2f corner0 = {frame2D.uvMin[0u] * imageSize.x, frame2D.uvMin[1u] * imageSize.y};
            const vec2f corner1 = {frame2D.uvMax[0u] * imageSize.x, frame2D.uvMax[1u] * imageSize.y};

            vec4f p0 = mvp * vec4f(corner0.x, corner0.y, 0.0f, 1.0f);
            vec4f p1 = mvp * vec4f(corner1.x, corner0.y, 0.0f, 1.0f);
            vec4f p2 = mvp * vec4f(corner0.x, corner1.y, 0.0f, 1.0f);
            vec4f p3 = mvp * vec4f(corner1.x, corner1.y, 0.0f, 1.0f);

            // todo: fill vertexes and indices
            // ....

            indexCount += 6u;
        }

        commandList->bindPipeline(pipeline);
        commandList->bindVertexBuffer(0u, (*_spritesVertices)(frame), 0u);
        commandList->bindIndexBuffer((*_spritesIndices)(frame), 0u, rhi::IndexType::eU32);
        commandList->bindImage(0u, image, rhi::SamplerValue{});

        commandList->drawIndexed(indexCount, 1u, _spritesIndicesCount);
        _spritesIndicesCount += indexCount;
    }
}