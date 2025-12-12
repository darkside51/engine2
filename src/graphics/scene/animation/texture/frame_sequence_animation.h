#pragma once

#include "graphics/texture/image_frame.h"

#include <span>
#include <vector>

namespace engine::scene {

    class FrameSequence {
    public:
        FrameSequence(float frameTime, std::vector<const graphics::ImageFrame2D *> frames) :
            _frameTime(frameTime), _frames(std::move(frames)) {}

        std::span<const graphics::ImageFrame2D * const> frames() const noexcept {
            return _frames;
        }

    private:
        std::vector<const graphics::ImageFrame2D *> _frames;
        float _frameTime = 1.0f / 30.0f; // 30fps as default
    };

}