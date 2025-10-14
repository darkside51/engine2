#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "node.h"
#include "object.h"
#include "objects_register.h"

#include <memory>
#include <unordered_map>
#include <utility>

namespace engine::scene {

struct ObjectsCollection {
    virtual ~ObjectsCollection() = default;
    virtual bool destroyObject(Object * object) noexcept { return false; }

    Object::Iterator begin() const noexcept {
        return {.value = first};
    }

    Object::Iterator end() const noexcept {
        return {.value = nullptr};
    }

    size_t size() const noexcept {
        size_t result = 0u;
        for (const auto & object : *this) {
            ++result;
        }
        return result;
    }

    bool empty() const noexcept {
        return first == last;
    }

    void clear() noexcept {
        first = nullptr;
        last = nullptr;
    }

    void push(Object * object) noexcept {
        if (first == nullptr) {
            first = object;
        } else {
            object->prev = last;
            last->next = object;
        }
        last = object;
        dirty = true;
    }

    void remove(Object * object) noexcept {
        if (object->prev) {
            object->prev->next = object->next;
        }
        if (object->next) {
            object->next->prev = object->prev;
        }
        if (last == object) {
            last = object->prev;
        }
        if (first == object) {
            first = object->next;
        }
    }

    // sorting the list (e.g., using Merge Sort)
    template <typename Comparator>
    void stable_sort(const Comparator & cmp) noexcept {
        // https://www.tutorialspoint.com/merge-sort-for-doubly-linked-list-using-cplusplus
        first = mergeSort(first, cmp);
        // update tail after sorting if needed, by traversing to the end
        Object* current = first;
        if (current) {
            while (current->next) {
                current = current->next;
            }
            last = current;
        } else {
            last = nullptr;
        }
    }

private:
    template <typename Comparator>
    Object* mergeSort(Object* h, const Comparator & cmp) noexcept {
        if (h == nullptr || h->next == nullptr) {
            return h; // base case: 0 or 1 node
        }

        Object* middle = getMiddle(h);
        Object* nextOfMiddle = middle->next;
        middle->next = nullptr; // split the list

        Object* left = mergeSort(h, cmp);
        Object* right = mergeSort(nextOfMiddle, cmp);

        return merge(left, right, cmp);
    }

    // helper function to find the middle node
    Object* getMiddle(Object* h) noexcept {
        Object* slow = h;
        Object* fast = h;
        while (fast->next != nullptr && fast->next->next != nullptr) {
            slow = slow->next;
            fast = fast->next->next;
        }
        return slow;
    }

    // helper function to merge two sorted lists
    template <typename Comparator>
    Object* merge(Object* first, Object* second, const Comparator & cmp) noexcept {
        if (first == nullptr) {
            return second;
        }
        if (second == nullptr) {
            return first;
        }

        Object* result = nullptr;
        if (!cmp(second, first)) {
            result = first;
            result->next = merge(first->next, second, cmp);
            if (result->next) {
                result->next->prev = result;
            }
            result->prev = nullptr;
        } else {
            result = second;
            result->next = merge(first, second->next, cmp);
            if (result->next) {
                result->next->prev = result;
            }
            result->prev = nullptr;
        }
        return result;
    }

public:
    Object * first = nullptr;
    Object * last = nullptr;
    bool dirty = false;
};

template<typename T>
struct ObjectsHolder : ObjectsCollection {
    using Type = T;
    using ObjectsAllocator = allocators::PoolAllocatorChain<sizeof(T), alignof(T), false>;
    ObjectsAllocator allocator{512u, 16u};

    template <typename... Args>
    Object * createObject(Args &&... args) noexcept {
        auto * object = allocator.template create<T>(std::forward<Args>(args)...);
        push(object);
        return object;
    }

    bool destroyObject(Object * object) noexcept override {
        remove(object);
        return allocator.destroy(object);
    }
};

class Scene {
public:
    Scene(debug::DebugDraw * debugDraw);
    ~Scene();

    Node * createNode() noexcept;
    void destroyNode(Node * node) noexcept;
    Node * rootNode() noexcept { return _rootNode; }

    void recalculateNodes(const mat4f & cameraTransform) noexcept;
    void draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame, const mat4f & cameraTransform);

    template <typename T, typename... Args>
    Object * createObject(Args &&... args) {
        constexpr auto id = ctti::typeIndex<Object, T>();
        auto it = _objectCollections.find(id);
        if (it == _objectCollections.end()) {
            auto [iter, _] = _objectCollections.emplace(id, std::pair{std::make_unique<ObjectsHolder<T>>(), Object::Iterator{}});
            it = iter;
        }

        auto & objectsData = static_cast<ObjectsHolder<T>&>(*it->second.first);
        return objectsData.createObject(std::forward<Args>(args)...);
    }

    bool destroyObject(Object * object) noexcept;

private:
    struct Layers {
        uint16_t minLayer = 0xff'ffu;
        uint16_t maxLayer = 0u;
    };
    Layers prepareObjects(ObjectsCollection & objects, size_t type) noexcept;
    Object::Iterator drawEmptyObjects(Object::Iterator it, Object::Iterator end, const mat4f & cameraTransform, uint32_t color, uint16_t layer);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false>;
    NodesAllocator _nodeAllocator{512u, 64u};
    Node * _rootNode = nullptr;

    std::unordered_map<size_t, std::pair<std::unique_ptr<ObjectsCollection>, Object::Iterator>> _objectCollections;

    debug::DebugDraw * _debugDraw = nullptr;
};

}