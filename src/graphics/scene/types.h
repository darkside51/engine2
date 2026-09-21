#pragma once

#include "math/math.h"
#include "stl/span.h"

#include <cstdint>

namespace engine::scene {

struct AttachmentObject;
using AttachmentsIterator = stl::span<AttachmentObject*>::iterator;

struct ShadowViewParams {
    stl::span<const mat4f> cascadeMatrices;
};

struct ViewParams {
    mat4f cameraView;
    mat4f cameraProjection;
    mat4f cameraTransform;
    mat3f viewTransposeInverse;
    vec3f cameraPosition;
    vec2f nearFarPlanes;
    vec2f cursorPosition;
    float time = 0.0f;
};

struct DrawDataOffsets {
    uint32_t meshesDrawOffset = 0u;
    uint32_t spriteVerticesOffset = 0u;
    uint32_t spriteIndicesOffset = 0u;
    uint32_t skeletonOffset = 0u;
    uint32_t computeSkinVerticesOffset = 0u;
    uint32_t colorIdMeshesOffset = 0u;
    uint32_t colorIdSkeletalMeshesOffset = 0u;
};

enum class FillAttachmentFlags : uint8_t {
    eNone = 0u,
    eCalculateQuality = 1u,
};

enum class RenderPass : uint8_t {
    eCommon = 0u,
    eShadow = 1u,
    eColorId = 2u
};

}