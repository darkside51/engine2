#pragma once

#include "graphics/texture/image_frame.h"

#include <span>
#include <string_view>
#include <vector>

namespace engine{

namespace graphics {
    class TextureManager;
}

namespace scene {

    class FramesSequence {
    public:
        explicit FramesSequence(std::vector<graphics::ImageFrame2D> frames) :
            _frames(std::move(frames)) {}

        FramesSequence(const graphics::TextureManager & textureManager, 
            std::span<std::string const> frames);

        std::span<graphics::ImageFrame2D const> frames() const noexcept {
            return _frames;
        }

    private:
        std::vector<graphics::ImageFrame2D> _frames;  
    };

    class FramesSequenceAnimator {
    public:
        explicit FramesSequenceAnimator(float frameTime) : _frameTime(frameTime) {}
        
        const graphics::ImageFrame2D & getFrame(float time, const FramesSequence & sequence) const noexcept;
        const graphics::ImageFrame2D & update(float delta, const FramesSequence & sequence) noexcept;

    private:
        float _frameTime = 1.0f / 30.0f; // 30fps as default
        float _time = 0.0f;
    };
}
}