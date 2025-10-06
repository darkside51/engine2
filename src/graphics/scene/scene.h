#pragma once

#include "allocators/pool_allocator_chain.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "node.h"

namespace engine::scene {

class Scene {
public:
    Scene(debug::DebugDraw * debugDraw);
    ~Scene();

    Node * createNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    void recalculateNodes(const mat4f & cameraTransform) noexcept;
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame);

private:
    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false>;
    NodesAllocator _nodeAllocator{512u, 64u};
    Node * _rootNode = nullptr;

    debug::DebugDraw * _debugDraw = nullptr;
};

}