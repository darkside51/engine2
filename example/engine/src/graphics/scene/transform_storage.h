#pragma once

#include "node.h"

#include "allocators/pool_allocator_chain.h"
#include "stl/vector.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace engine::scene {

    // Pool of TransformChunk instances. Nodes keep a (chunk, slot) pair instead of
    // owning transform data inline, so the hot node stays one cache line.
    class TransformStorage {
    public:
        struct Handle {
            TransformChunk * chunk = nullptr;
            uint16_t slot = 0u;
        };

        TransformStorage() = default;
        ~TransformStorage() noexcept {
            clear();
        }

        TransformStorage(const TransformStorage &) = delete;
        TransformStorage & operator=(const TransformStorage &) = delete;

        Handle acquire() noexcept {
            if (!_freeChunks.empty()) {
                auto * chunk = _freeChunks.back();
                uint16_t slot = 0u;

                if (chunk->acquire(slot)) {
                    if (chunk->isFull()) {
                        _freeChunks.pop_back();
                    }
                    chunk->init(slot);
                    return Handle{chunk, slot};
                }

                _freeChunks.pop_back(); // stale entry, should not happen
            }

            auto * chunk = _allocator.template create<TransformChunk>();
            assert(chunk != nullptr);
            _chunks.emplace_back(chunk);

            uint16_t slot = 0u;
            [[maybe_unused]] const bool acquired = chunk->acquire(slot);
            assert(acquired);
            chunk->init(slot);

            if (!chunk->isFull()) {
                _freeChunks.emplace_back(chunk);
            }

            return Handle{chunk, slot};
        }

        void release(TransformChunk * chunk, uint16_t slot) noexcept {
            if (chunk == nullptr) {
                return;
            }

            const bool wasFull = chunk->isFull();
            chunk->release(slot);

            if (wasFull) {
                _freeChunks.emplace_back(chunk);
            }
        }

        void clear() noexcept {
            for (auto * chunk : _chunks) {
                _allocator.destroy(chunk);
            }
            _chunks.clear();
            _freeChunks.clear();
        }

        size_t chunksCount() const noexcept {
            return _chunks.size();
        }

    private:
        // 16 TransformChunk per pool of 128 chuncks, up to 2048 chunks => 131'072 nodes
        using Allocator = allocators::PoolAllocatorChain<sizeof(TransformChunk), alignof(TransformChunk), true, false>;
        Allocator _allocator{16u, 128u};
        stl::vector<TransformChunk*> _chunks;
        stl::vector<TransformChunk*> _freeChunks;
    };

}
