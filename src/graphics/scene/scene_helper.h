#pragma once

#include "bounds.h"

#include <cstdint>

namespace engine{

namespace graphics {
    class ModelManager;
}
namespace scene {
    class Scene;
    struct Node;

    Node * createNodeWithSkeletalMesh(Scene & scene, const graphics::ModelManager & modelManager, uint32_t modelIndex, Bounds::Type boundsType);
}

}