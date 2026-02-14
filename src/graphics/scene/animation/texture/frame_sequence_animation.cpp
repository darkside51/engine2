#include "frame_sequence_animation.h"

#include "common/type_id.h"
#include "graphics/scene/register.h"
#include "graphics/texture/texture_manager.h"

namespace engine::scene {
    
    FramesSequence::FramesSequence(std::vector<graphics::ImageFrame2D> frames) : 
        Animation(ctti::typeIndex<Animation, FramesSequence>()), 
        _frames(std::move(frames)) {}

    FramesSequence::FramesSequence(const graphics::TextureManager & textureManager, 
        std::span<std::string const> frames) : Animation(ctti::typeIndex<Animation, FramesSequence>()) {
        _frames.reserve(frames.size());
        for (auto frameName : frames) {
            _frames.emplace_back(textureManager.findFrame(frameName));
        }
    }

    const graphics::ImageFrame2D & FramesSequenceAnimator::getFrame(float time, const FramesSequence & sequence) const noexcept {
        const auto frames = sequence.frames(); // span
        const auto frame = static_cast<uint32_t>(time / _frameTime) % frames.size();
        return frames[frame];
    }

    const graphics::ImageFrame2D & FramesSequenceAnimator::update(float delta, const FramesSequence & sequence) noexcept {
        _time += delta;
        return getFrame(_time, sequence);
    }

    const graphics::ImageFrame2D & FramesSequenceAnimator::update2(float time, const FramesSequence & sequence) const noexcept {
        return getFrame(time, sequence);
    }
}