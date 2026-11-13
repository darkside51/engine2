#pragma once

#include "../core/system.h"
#include "../graphics/scene/transform_storage.h"
#include "../log/log.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace engine::tests {

    struct NodeTest {
        static constexpr uint32_t kNodesCount = 100'000u;
        static constexpr uint32_t kIterations = 20u;
        static constexpr uint32_t kChildrenCount = 10'000u;

        using Clock = std::chrono::steady_clock;
        using Node = engine::scene::Node;
        using Bounds = engine::scene::Bounds;
        using TransformStorage = engine::scene::TransformStorage;

        template <typename F>
        static double measureMs(F && f) {
            const auto begin = Clock::now();
            f();
            return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
        }

        Node * createNode() {
            const auto handle = _storage.acquire();
            return new Node(handle.chunk, handle.slot);
        }

        Node * buildFlatTree(uint32_t count) {
            auto * root = createNode();
            root->setId(0u);

            Node * prev = nullptr;
            for (uint32_t i = 0u; i < count; ++i) {
                auto * node = createNode();
                node->parent = root;
                node->setId(i + 1u);
                node->setBounds(Bounds::createCube(engine::vec3f{-1.0f}, engine::vec3f{1.0f}));

                if (prev) {
                    prev->next = node;
                } else {
                    root->firstChild = node;
                }

                prev = node;
            }

            return root;
        }

        static void destroyTree(Node * node) noexcept {
            while (node) {
                Node * next = node->next;
                destroyTree(node->firstChild);
                delete node;
                node = next;
            }
        }

        static void logLayout() {
            LOG_I("Node layout:");
            LOG_I("  sizeof(Node)=%zu alignof(Node)=%zu cacheline=%u",
                sizeof(Node), alignof(Node), kCachelineSize);
            LOG_I("  sizeof(TransformChunk)=%zu per node=%.1f B",
                sizeof(engine::scene::TransformChunk),
                static_cast<double>(sizeof(engine::scene::TransformChunk)) / engine::scene::kTransformChunkSize);
            LOG_I("  parent=%zu next=%zu firstChild=%zu attachment=%zu",
                offsetof(Node, parent),
                offsetof(Node, next),
                offsetof(Node, firstChild),
                offsetof(Node, attachment));
            LOG_I("  transforms=%zu slot=%zu id=%zu state=%zu",
                offsetof(Node, transforms),
                offsetof(Node, slot),
                offsetof(Node, id),
                offsetof(Node, state));
            LOG_I("  node footprint: %.2f MB for %u nodes",
                static_cast<double>(kNodesCount) * sizeof(Node) / (1024.0 * 1024.0),
                kNodesCount);
        }

        void testReParenting() {
            Node * p1 = createNode();
            Node * p2 = createNode();
            Node * a = createNode();
            Node * b = createNode();

            p1->addChild(a);
            p1->addChild(b);
            assert(p1->firstChild == a);
            assert(a->next == b);
            assert(b->next == nullptr);

            // removing the first child must fully detach it
            assert(p1->removeChild(a));
            assert(a->parent == nullptr);
            assert(a->next == nullptr);
            assert(p1->firstChild == b);
            assert(b->next == nullptr);

            // re-parenting must not drag the old sibling list along
            p2->addChild(a);
            assert(p2->firstChild == a);
            assert(a->parent == p2);
            assert(a->next == nullptr);
            assert(p1->firstChild == b);
            assert(b->next == nullptr);

            assert(p2->removeChild(a));
            assert(p1->removeChild(b));

            delete a;
            delete b;
            delete p1;
            delete p2;
        }

        static void testBounds() {
            const auto sphere = Bounds::createSphere(engine::vec3f{1.0f, 2.0f, 3.0f}, 0.5f);
            assert(sphere.type == Bounds::Type::eSphere);
            assert(sphere.volume.sphere.center.x == 1.0f);
            assert(sphere.volume.sphere.center.y == 2.0f);
            assert(sphere.volume.sphere.center.z == 3.0f);
            assert(sphere.volume.sphere.radius == 0.5f);

            const auto transformed = sphere.transform(engine::mat4f{1.0f});
            assert(transformed.type == Bounds::Type::eSphere);
            assert(transformed.volume.sphere.radius == 0.5f);
        }

        static void testDirtyFlags(Node * root) {
            const auto visible = [](Node *) noexcept { return true; };

            engine::scene::recalculateHierarchy(root, visible);

            // after a pass no node may keep the "children dirty" flag
            engine::scene::hierarchyPass(root, [](Node * n) noexcept {
                assert(!n->isChildrenTransformsDirty());
            });

            // a clean subtree must not be recomputed on the next pass
            Node * child = root->firstChild;
            assert(child != nullptr);
            const_cast<engine::mat4f &>(child->getWorldTransform())[3u].x = 12345.0f;

            engine::scene::recalculateHierarchy(root, visible);
            assert(child->getWorldTransform()[3u].x == 12345.0f);
        }

        void main() {
            logLayout();

            testReParenting();
            testBounds();

            auto * root = buildFlatTree(kNodesCount);
            testDirtyFlags(root);

            const auto visible = [](Node *) noexcept { return true; };

            double traversalMs = 0.0;
            for (uint32_t i = 0u; i < kIterations; ++i) {
                traversalMs += measureMs([&]() {
                    engine::scene::recalculateHierarchy(root, visible);
                });
            }

            double cullMs = 0.0;
            float accumulator = 0.0f;
            for (uint32_t i = 0u; i < kIterations; ++i) {
                cullMs += measureMs([&]() {
                    engine::scene::hierarchyPass(root, [&accumulator](Node * n) noexcept {
                        const auto & world = n->getWorldTransform();
                        const auto & bounds = n->getBounds();
                        if (bounds.type != Bounds::Type::eNone) {
                            const auto transformed = bounds.transform(world);
                            accumulator += transformed.volume.cube.corner1.x;
                        }
                    });
                });
            }

            LOG_I("Node traversal: %.4f ms/pass for %u nodes", traversalMs / kIterations, kNodesCount);
            LOG_I("Node cull+bounds: %.4f ms/pass (acc=%.1f)", cullMs / kIterations, static_cast<double>(accumulator));

            const double addChildMs = measureMs([this]() {
                auto * parent = createNode();
                for (uint32_t i = 0u; i < kChildrenCount; ++i) {
                    parent->addChild(createNode());
                }
                destroyTree(parent);
            });

            LOG_I("addChild x%u append-to-tail: %.4f ms", kChildrenCount, addChildMs);

            destroyTree(root);
            _storage.clear();
        }

    private:
        TransformStorage _storage;
    };

}
