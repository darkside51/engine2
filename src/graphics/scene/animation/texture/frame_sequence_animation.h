#pragma once

#include "graphics/scene/animation.h"
#include "graphics/texture/image_frame.h"

#include <chrono>
#include <span>
#include <string_view>
#include <vector>

namespace engine{

namespace graphics {
    class TextureManager;
}

namespace scene {

    class FramesSequence : public Animation {
    public:
        explicit FramesSequence(std::vector<graphics::ImageFrame2D> frames);

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
        const graphics::ImageFrame2D & update(std::chrono::duration<float> now, float startTime, const FramesSequence & sequence) const noexcept;

    private:
        float _frameTime = 1.0f / 30.0f; // 30fps as default
        float _time = 0.0f;
    };
}
}