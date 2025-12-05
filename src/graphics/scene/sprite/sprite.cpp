#include "sprite.h"

#include "common/type_id.h"
#include "graphics/scene/objects_register.h"
#include "graphics/scene/node.h"

#include <functional>

namespace engine::scene {

    Sprite::Sprite(const graphics::ImageFrame2D & frame, uint32_t pipeline) : 
        Object(ctti::typeIndex<Object, Sprite>()), _frame(&frame), _pipeline(pipeline) {
    }

    void Sprite::setPipeline(uint32_t pipeline) noexcept {
        _pipeline = pipeline;
    }

    uint32_t Sprite::getPipeline() const noexcept { 
        return _pipeline;
    }

    void Sprite::setFrame(const graphics::ImageFrame2D & frame) noexcept {
        _frame = &frame;
        if (node) {
            const auto spriteSize = calculateSize();
            node->setBounds(Bounds::createCube(vec3f{0.0f}, vec3f{spriteSize.x, spriteSize.y, 0.0f}));
        }
    }

    const graphics::ImageFrame2D & Sprite::getFrame() const noexcept {
        return *_frame;
    }

    vec2f Sprite::calculateSize() const noexcept {
        const auto & extent = _frame->getImage()->getExtent();
        const auto & frame = _frame->getFrame();
        return vec2f{frame.uvMax[0u] - frame.uvMin[0u], frame.uvMax[1u] - frame.uvMin[1u]} * vec2f{extent.width, extent.height};
    }

    bool Sprite::cmp(const Sprite & other) const noexcept {
        if (_pipeline != other._pipeline) {
            return _pipeline < other._pipeline;
        }

        return std::less<const rhi::runtime::Image*>{}(_frame->getImage().get(), other._frame->getImage().get());
    }

    void Sprite::attachToNode(Node * node) noexcept {
        if (node) {
            const auto spriteSize = calculateSize();
            node->setObject(this);
            node->setBounds(Bounds::createCube(vec3f{0.0f}, vec3f{spriteSize.x, spriteSize.y, 0.0f}));
        }
    }

}