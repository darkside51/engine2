#pragma once

#include "math/math.h"
#include "bounds.h"
#include "core/system.h"

#include <cassert>
#include <cstdint>

namespace engine::scene {

constexpr uint8_t kDirtyTransformBit = 0u;
constexpr uint8_t kCulledBit = 1u;
constexpr uint8_t kHiddenBit = 2u;
constexpr uint8_t kChildrenTransformsDirty = 3u;
constexpr uint8_t kTransformationsDirty = 4u;

constexpr uint8_t kDefaultState = 1u << kDirtyTransformBit | 
                                0u << kCulledBit | 0u << kHiddenBit | 
                                0u << kChildrenTransformsDirty | 0u << kTransformationsDirty;

struct AttachmentObject;

// nodes per transform chunk (one chunk is ~11 KB, see TransformChunk below)
constexpr uint32_t kTransformChunkSize = 64u;

// SoA transform storage: one chunk keeps hot transform data of kTransformChunkSize nodes.
// Consecutive nodes allocated from the same chunk have consecutive slots, so culling
// and animation passes stream through the arrays instead of jumping over fat nodes.
struct alignas(kCachelineSize) TransformChunk {
    mat4f world[kTransformChunkSize];       // hot for culling / rendering / skinning
    mat4f local[kTransformChunkSize];       // cached TRS composition, rebuilt only when TRS is dirty
    vec3f position[kTransformChunkSize];    // hot for animation
    quatf rotation[kTransformChunkSize];
    vec3f scale[kTransformChunkSize];
    vec3f pivot[kTransformChunkSize];
    Bounds bounds[kTransformChunkSize];     // hot for culling
    uint64_t occupied = 0u;                 // one bit per slot

    inline bool acquire(uint16_t & slot) noexcept {
        if (occupied == ~uint64_t{0u}) {
            return false;
        }

        // first FREE slot: ctz over the inverted mask, not over the occupied bits
        const uint64_t freeSlots = ~occupied;

#if defined(__GNUC__) || defined(__clang__)
        slot = static_cast<uint16_t>(__builtin_ctzll(freeSlots)); // (count trailing zeros — counting the number of zeros at the end)
#elif defined(_MSC_VER) && defined(_M_X64)
        unsigned long index = 0ul;
        _BitScanForward64(&index, freeSlots);
        slot = static_cast<uint16_t>(index);
#else
        slot = 0u;
        while ((freeSlots & (uint64_t{1u} << slot)) == 0u) {
            ++slot;
        }
#endif
        occupied |= uint64_t{1u} << slot;
        return true;
    }

    inline void release(uint16_t slot) noexcept {
        occupied &= ~(uint64_t{1u} << slot);
    }

    inline void init(uint16_t slot) noexcept {
        world[slot] = mat4f{1.0f};
        local[slot] = mat4f{1.0f};
        position[slot] = vec3f{0.0f};
        rotation[slot] = quatf{1.0f, 0.0f, 0.0f, 0.0f};
        scale[slot] = vec3f{1.0f};
        pivot[slot] = vec3f{0.0f};
        bounds[slot] = Bounds{};
    }

    inline bool isFull() const noexcept {
        return occupied == ~uint64_t{0u};
    }
};

// hot node data: exactly one cache line, everything the hierarchy walk touches
// cold transform data lives in TransformChunk and is addressed by (transforms, slot)
struct alignas(kCachelineSize) Node {
    inline static constexpr uint32_t kUnknownId = 0xff'ff'ff'ffu;
    struct State {
        uint8_t state = kDefaultState;

        inline bool isCulled() const noexcept {
            return state & (1u << kCulledBit);
        }

        inline bool isHidden() const noexcept {
            return state & (1u << kHiddenBit);
        }

        inline bool isTransformDirty() const noexcept {
            return state & (1u << kDirtyTransformBit);
        }

        inline bool isChildrenTransformsDirty() const noexcept {
            return state & (1u << kChildrenTransformsDirty);
        }

        inline bool isTransformationsDirty() const noexcept {
            return state & (1u << kTransformationsDirty);
        }

        inline void setCulled(bool culled) noexcept {
            culled ? state |= (1u << kCulledBit) : state &= ~(1u << kCulledBit);
        }

        inline void setHidden(bool hidden) noexcept {
            if (!hidden && isHidden()) {
                setTransformDirty(true);
            }
            hidden ? state |= (1u << kHiddenBit) : state &= ~(1u << kHiddenBit);
        }

        inline void setTransformDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kDirtyTransformBit) : state &= ~(1u << kDirtyTransformBit);
        }

        inline void setChildrenTransformsDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kChildrenTransformsDirty) : state &= ~(1u << kChildrenTransformsDirty);
        }

        inline void setTransformationsDirty(bool dirty) noexcept {
            dirty ? state |= (1u << kTransformationsDirty) : state &= ~(1u << kTransformationsDirty);
        }
    };

    Node(TransformChunk * transformChunk, uint16_t transformSlot) noexcept :
        transforms{transformChunk}, slot{transformSlot} {}

    Node(const Node &) = delete;
    Node & operator=(const Node &) = delete;

    Node * parent = nullptr;
    Node * next = nullptr; // other childrens in parent (brothers)
    Node * firstChild = nullptr; // children begin
    AttachmentObject * attachment = nullptr;

    TransformChunk * transforms = nullptr; // cold transform data owner
    uint32_t id = kUnknownId;
    uint16_t slot = 0u;
    State state{kDefaultState};

    uint32_t getId() const noexcept { return id; }
    void setId(uint32_t nodeId) noexcept { id = nodeId; }

    void setTransform(const mat4f & m) noexcept;
    void applyTransformations() noexcept;
    bool calculateWorldTransform(bool recalculate) noexcept;

    void setBounds(const Bounds & newBounds) noexcept {
        transforms->bounds[slot] = newBounds;
    }

    const Bounds & getBounds() const noexcept {
        return transforms->bounds[slot];
    }

    // attachment
    void setAttachment(AttachmentObject * object) noexcept;
    AttachmentObject * removeAttachment() noexcept;
    AttachmentObject * getAttachment() const noexcept;
    AttachmentObject * getAttachment(uint8_t idx) const noexcept;

    // state
    inline bool isCulled() const noexcept {
        return state.isCulled();
    }

    inline bool isHidden() const noexcept {
        return state.isHidden();
    }

    inline bool isTransformDirty() const noexcept {
        return state.isTransformDirty();
    }

    inline bool isVisible() const noexcept {
        return !state.isCulled() && !state.isHidden();
    }

    inline bool isChildrenTransformsDirty() const noexcept {
        return state.isChildrenTransformsDirty();
    }

    inline void setCulled(bool culled) noexcept {
        state.setCulled(culled);
    }
    
    inline void setHidden(bool hidden) noexcept {
        state.setHidden(hidden);
    }

    inline void setTransformDirty(bool dirty) noexcept {
        state.setTransformDirty(dirty);
    }

    inline void setChildrenTransformsDirty(bool dirty) noexcept {
        state.setChildrenTransformsDirty(dirty);
    }

    const mat4f & getLocalTransform() const noexcept {
        return transforms->local[slot];
    }

    const mat4f & getWorldTransform() const noexcept {
        return transforms->world[slot];
    }

    // children
    void addChild(Node * child) noexcept;
    bool removeChild(Node * child) noexcept;
    bool removeFromParent() noexcept;

    Node * getChild(uint32_t nodeId) const noexcept;
    Node * findChildInHierarchy(uint32_t nodeId) const noexcept;
    Node * childAt(uint32_t index) const noexcept;

    // transformation
    const quatf & getRotation() const noexcept { return transforms->rotation[slot]; }
    vec3f getRotationAngles() const noexcept { return glm::eulerAngles(transforms->rotation[slot]); }
    const vec3f & getScale() const noexcept { return transforms->scale[slot]; }
    const vec3f & getPosition() const noexcept { return transforms->position[slot]; }
    const vec3f & getPivot() const noexcept { return transforms->pivot[slot]; }

    void setPosition(const vec3f & p) noexcept {
        if (compare(transforms->position[slot], p, 1e-7f)) {
            transforms->position[slot] = p;
            state.setTransformationsDirty(true);
        }
    }

    void setScale(const vec3f & s) noexcept {
        if (compare(transforms->scale[slot], s, 1e-7f)) {
            transforms->scale[slot] = s;
            state.setTransformationsDirty(true);
        }
    }

    void setScale(float s) noexcept {
        setScale(vec3f{s});
    }

    void setRotation(const quatf & q) noexcept {
        if (compare(transforms->rotation[slot], q, 1e-5f)) {
            transforms->rotation[slot] = q;
            state.setTransformationsDirty(true);
        }
    }

    void setRotation(const vec3f & angles) noexcept {
        setRotation(quatf{angles});
    }

    void setPivot(const vec3f & p) noexcept {
        if (compare(transforms->pivot[slot], p, 1e-7f)) {
            transforms->pivot[slot] = p;
            state.setTransformationsDirty(true);
        }
    }
};

static_assert(sizeof(Node) == kCachelineSize, "Node must fit exactly one cache line");
static_assert(alignof(Node) == kCachelineSize, "Node must be cache line aligned");

template <typename Function>
void childrenPass(Node * root, Function && f) noexcept {
    Node * node = root->firstChild;
    while (node != nullptr) {
        f(node);
        node = node->next;
    }
}

template <typename Function>
void hierarchyPass(Node * root, Function && f) noexcept {
    Node * node = root;
    bool process = true;
    
    while (node != nullptr) {
        if (process) {
            f(node);
        }
        
        if (node->isVisible() && node->firstChild && process) {
            node = node->firstChild;
            process = true;
            continue;
        }
        
        if (node == root) {
            break;
        }

        if (node->next != nullptr) {
            node = node->next;
            process = true;
        } else {
            node = node->parent;
            process = false;
        }
    }
}

template <typename VisibleChecker>
void recalculateHierarchy(Node * root, VisibleChecker && v) noexcept {
    Node * node = root;
    bool process = true;

    while (node != nullptr) {
        prefetchMemory<MemoryPrefetchMode::eRead, MemoryLocality::eHigh>(node->next);
        prefetchMemory<MemoryPrefetchMode::eRead, MemoryLocality::eHigh>(node->firstChild);

        if (process && !node->isHidden()) {
            node->applyTransformations();

            const bool parentDirty = node->parent && node->parent->isChildrenTransformsDirty();
            if (node->calculateWorldTransform(parentDirty)) {
                node->setChildrenTransformsDirty(true);
            }
            
            node->setCulled(!v(node));
        }

        if (process && !node->isHidden() && node->firstChild) {
            node = node->firstChild;
            process = true;
            continue;
        }
        
        if (node == root) {
            node->setChildrenTransformsDirty(false); // root subtree has been processed
            break;
        }

        // exiting the node: its subtree has been fully processed;
        // the children have already read the flag, so it is no longer needed
        node->setChildrenTransformsDirty(false);
        
        if (node->next != nullptr) {
            node = node->next;
            process = true;
        } else {
            node = node->parent;
            process = false;
        }
    }
}

inline vec3f toWorld(const vec3f & point, const mat4f & worldTransform) noexcept {
    const auto v = worldTransform * vec4f{point, 1.0f};
    return v / v.w;
}

inline vec3f toNode(const vec3f & worldPoint, const mat4f & inverseWorldTransform) noexcept {
    const auto v = inverseWorldTransform * vec4f{worldPoint, 1.0f};
    return v / v.w;
}

inline vec3f nodeToWorld(const vec3f & point, const Node & node) noexcept {
    return toWorld(point, node.getWorldTransform());
}

inline vec3f worldToNode(const vec3f & worldPoint, const Node & node) noexcept {
    return toNode(worldPoint, glm::inverse(node.getWorldTransform()));
}

}
