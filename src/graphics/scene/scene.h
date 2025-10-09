#pragma once

#include "allocators/pool_allocator_chain.h"
#include "common/type_id.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/debug/debug_draw.h"
#include "math/math.h"
#include "node.h"
#include "object.h"
#include "objects_register.h"

#include <unordered_map>

namespace engine::scene {

struct ObjectsCollection {
    virtual ~ObjectsCollection() = default;
    virtual bool destroyObject(Object * object) noexcept = 0;

    struct Iterator {
        Object * value = nullptr;
        Iterator& operator++() noexcept {
            value = value->next;
            return *this;
        }
        Iterator& operator--() noexcept {
            value = value->prev;
            return *this;
        }

        bool operator==(const Iterator &) const noexcept = default;
        Object * operator->() noexcept { return value; }
        Object & operator*() noexcept { return *value; }
        const Object * operator->() const noexcept { return value; }
        const Object & operator*() const noexcept { return *value; }
    };

    Iterator begin() const noexcept {
        return Iterator{.value = first};
    }

    Iterator end() const noexcept {
        return Iterator{.value = nullptr};
    }

    // sorting the list (e.g., using Merge Sort)
    template <typename Comparator>
    void stable_sort(Comparator && cmp) noexcept {
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
    Object* mergeSort(Object* h, Comparator && cmp) noexcept {
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
    Object* merge(Object* first, Object* second, Comparator && cmp) noexcept {
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
        } else {
            result = second;
            result->next = merge(first, second->next, cmp);
            if (result->next) {
                result->next->prev = result;
            }
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
    ObjectsAllocator allocator{512u, 64u};

    bool destroyObject(Object * object) noexcept override {
        if (object->prev) {
            object->prev->next = object->next;
        }
        if (object->next) {
            object->next->prev = object->prev;
        }
        if (last == object) {
            last = object->prev ? object->prev : object->next;
        }
        if (first == object) {
            first = object->next;
        }
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
            auto [iter, _] = _objectCollections.emplace(id, new ObjectsHolder<T>{});
            it = iter;
        }

        auto & objectsData = *static_cast<ObjectsHolder<T>*>(it->second);

        auto * object = objectsData.allocator.template create<T>(std::forward<Args>(args)...);
        if (objectsData.first == nullptr) {
            objectsData.first = object;
        } else {
            object->prev = objectsData.last;
            objectsData.last->next = object;
        }
        objectsData.last = object;
        objectsData.dirty = true;

        return object;
    }

    bool destroyObject(Object * object) noexcept;

private:
    void drawEmptyObjects(ObjectsCollection & objects, const mat4f & cameraTransform, uint32_t color);

    using NodesAllocator = allocators::PoolAllocatorChain<sizeof(Node), alignof(Node), false>;
    NodesAllocator _nodeAllocator{512u, 64u};
    Node * _rootNode = nullptr;

    std::unordered_map<size_t, ObjectsCollection*> _objectCollections;

    debug::DebugDraw * _debugDraw = nullptr;
};

}