#include "sprite.h"

#include "common/type_id.h"
#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

#include <functional>

namespace engine::scene {

    Sprite::Sprite(const graphics::ImageFrame2D & frame, uint32_t pipeline) : 
        AttachmentObject(ctti::typeIndex<AttachmentObject, Sprite>()), _frame(frame), _pipeline(pipeline) {
    }

    void Sprite::setPipeline(uint32_t pipeline) noexcept {
        _pipeline = pipeline;
    }

    uint32_t Sprite::getPipeline() const noexcept { 
        return _pipeline;
    }

    void Sprite::setFrame(const graphics::ImageFrame2D & frame) noexcept {
        _frame = frame;
        if (node) {
            const auto spriteSize = calculateSize();
            node->setBounds(Bounds::createCube(vec3f{0.0f}, vec3f{spriteSize.x, spriteSize.y, 0.0f}));
        }
    }

    const graphics::ImageFrame2D & Sprite::getFrame() const noexcept {
        return _frame;
    }

    vec2f Sprite::calculateSize() const noexcept {
        if (_size.x != std::numeric_limits<float>::max() || _size.y != std::numeric_limits<float>::max()) {
            return _size;
        }
        return _frame.getSize();
    }

    vec2f Sprite::getSize() const noexcept {
        return calculateSize();
    }

    bool Sprite::cmp(const Sprite & other) const noexcept {
        if (_pipeline != other._pipeline) {
            return _pipeline < other._pipeline;
        }

        return std::less<const rhi::runtime::Image*>{}(_frame.getImage().get(), other._frame.getImage().get());
    }

    void Sprite::attachToNode(Node * node) noexcept {
        if (node) {
            const auto spriteSize = calculateSize();
            node->setAttachment(this);
            node->setBounds(Bounds::createCube(vec3f{0.0f}, vec3f{spriteSize.x, spriteSize.y, 0.0f}));
        }
    }

}