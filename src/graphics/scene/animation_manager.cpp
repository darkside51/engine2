#include "animation_manager.h"

namespace engine::scene {

void AnimationManager::destroy() noexcept {
    _animationNames.clear();
    _sequences.clear();
}

}