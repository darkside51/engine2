#include "frame_sequence_animation.h"

#include "graphics/texture/texture_manager.h"

namespace engine::scene {
    
    FramesSequence::FramesSequence(const graphics::TextureManager & textureManager, 
        std::span<std::string const> frames) {
        _frames.reserve(frames.size());
        for (auto frameName : frames) {
            if (auto * frame = textureManager.findFrame(frameName)) {
                _frames.emplace_back(*frame);
            }
        }
    }

    const graphics::ImageFrame2D & FramesSequenceAnimator::getFrame(float time, const FramesSequence & sequence) const noexcept {
        const auto frames = sequence.frames();
        const auto frame = static_cast<uint32_t>(time / _frameTime) % frames.size();
        return frames[frame];
    }

    const graphics::ImageFrame2D & FramesSequenceAnimator::update(float delta, const FramesSequence & sequence) noexcept {
        _time += delta;
        return getFrame(_time, sequence);
    }

}