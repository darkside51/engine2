#include "animation_manager.h"

namespace engine::scene {

void AnimationManager::destroy() noexcept {
    _animationNames.clear();
    _skeletalAnimations.clear();
    _sequences.clear();
}

}