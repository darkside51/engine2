#pragma once

#include "graphics/scene/animation.h"
#include "graphics/texture/image_frame.h"

#include <chrono>
#include <span>
#include <vector>

namespace engine {

namespace graphics {
    class TextureManager;
}

namespace scene {

    class FramesSequence : public Animation {
    public:
        ~FramesSequence() = default;
        explicit FramesSequence(stl::vector<graphics::ImageFrame2D> frames);

        FramesSequence(const graphics::TextureManager & textureManager, 
            stl::span<std::string const> frames);

        FramesSequence(FramesSequence&&) = default;
        FramesSequence& operator= (FramesSequence&&) = default;

        stl::span<graphics::ImageFrame2D const> frames() const noexcept {
            return _frames;
        }

    private:
        stl::vector<graphics::ImageFrame2D> _frames;  
    };

    class FramesSequenceAnimator {
    public:
        explicit FramesSequenceAnimator(float frameTime) : _frameTime(frameTime) {}
        
        const graphics::ImageFrame2D & getFrame(float time, const FramesSequence & sequence) const noexcept;
        const graphics::ImageFrame2D & update(float delta, const FramesSequence & sequence) noexcept;
        const graphics::ImageFrame2D & update2(float time, const FramesSequence & sequence) const noexcept;

    private:
        float _frameTime = 1.0f / 30.0f; // 30fps as default
        float _time = 0.0f;
    };
}
}