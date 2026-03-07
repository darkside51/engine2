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

    struct MeshPipelines {
        uint32_t commonPipeline = 0u;
        uint32_t skeletalPipeline = 0u;
    };
    Node * createNodeWithSkeletalMesh(Scene & scene, const graphics::ModelManager & modelManager, uint32_t modelIndex, Bounds::Type boundsType, 
        MeshPipelines pipelines, int16_t order = 0);
}

}