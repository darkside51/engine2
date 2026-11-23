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

    struct ObjectSizeLayout {
        enum class Type : uint8_t {
            eNone = 0u,
            eStrict = 1u,
            eMin = 2u,
            eMax = 3u
        };

        float size[3u] = {1.0f, 1.0f, 1.0f};
        Type type = Type::eStrict;
    };

    struct MeshPipelines {
        uint32_t commonPipeline = 0u;
        uint32_t skeletalPipeline = 0u;
    };
    Node * createNodeWithSkeletalMesh(Scene & scene, const graphics::ModelManager & modelManager, uint32_t modelIndex, Bounds::Type boundsType, 
        MeshPipelines pipelines, ObjectSizeLayout layout = {}, int16_t order = 0, uint32_t nodeId = 0xff'ff'ff'ffu);
}

}