#include "sprite.h"
#include "graphics/scene/objects_register.h"
#include "common/type_id.h"

#include <functional>

namespace engine::scene {

    Sprite::Sprite(const graphics::ImageFrame2D & frame, uint32_t pipeline) : 
        Object(ctti::typeIndex<Object, Sprite>()), _frame(frame), _pipeline(pipeline) {

    }

    void Sprite::setPipeline(uint32_t pipeline) noexcept {
        _pipeline = pipeline;
    }

    uint32_t Sprite::getPipeline() const noexcept { 
        return _pipeline;
    }

    void Sprite::setFrame(const graphics::ImageFrame2D & frame) noexcept {
        _frame = frame;
    }

    const graphics::ImageFrame2D & Sprite::getFrame() const noexcept {
        return _frame;
    }

    bool Sprite::cmp(const Sprite & other) const noexcept {
        if (_pipeline != other._pipeline) {
            return _pipeline < other._pipeline;
        }

        return std::less<const rhi::runtime::Image*>{}(_frame.getImage().get(), other._frame.getImage().get());
    }

}