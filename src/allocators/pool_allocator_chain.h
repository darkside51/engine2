#pragma once

#include "pool_allocator.h"
#include "threads/threads.h"
#include "stl/list.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace allocators {

    template<size_t bytes, size_t align, bool use_meta, bool thread_safe>
    class PoolAllocatorChain;

    struct PoolChainAllocationMetaInfo {
        void * pool = nullptr;
    };

    template<size_t size, size_t align>
    struct AlignmentInfo {
        friend class PoolAllocatorChain<size, align, true, false>;
        friend class PoolAllocatorChain<size, align, true, true>;

        static constexpr uintptr_t kMetaAlignMask = alignof(PoolChainAllocationMetaInfo) - 1u;
        static constexpr uintptr_t kAllocationSize = ((size + sizeof(PoolChainAllocationMetaInfo)) + kMetaAlignMask) & ~kMetaAlignMask;
        static constexpr uintptr_t kAllocationAlign = std::max(align, alignof(PoolChainAllocationMetaInfo));

    private:
        inline static PoolChainAllocationMetaInfo * metaInfo(void * m) noexcept {
            return reinterpret_cast<PoolChainAllocationMetaInfo*>(
                reinterpret_cast<uintptr_t>(
                    (reinterpret_cast<std::byte*>(m) + size) + kMetaAlignMask) & ~kMetaAlignMask
                );
        }
    };

    template <typename Allocator, typename Counter>
    struct PoolChunk {
        Allocator allocator;
        Counter allocations = {0u};

        explicit PoolChunk(size_t capacity) : allocator(capacity) {}

        inline size_t used_memory() const noexcept {
            return sizeof(PoolChunk) + allocator.used_memory();
        }

        PoolChunk(PoolChunk &&) noexcept = default;
        PoolChunk(const PoolChunk &) = delete;

        PoolChunk & operator=(PoolChunk && pool) = delete;
        PoolChunk & operator=(const PoolChunk & pool) = delete;
    };

    template <size_t bytes, size_t align, bool use_meta>
    struct AllocatorType;

    template <size_t bytes, size_t align>
    struct AllocatorType<bytes, align, false> {
        template <bool thread_safe>
        using Allocator = PoolAllocator<bytes, align, thread_safe>;
    };

    template <size_t bytes, size_t align>
    struct AllocatorType<bytes, align, true> {
        using AlignInfo = AlignmentInfo<bytes, align>;
        template <bool thread_safe>
        using Allocator = PoolAllocator<AlignInfo::kAllocationSize, AlignInfo::kAllocationAlign, thread_safe>;
    };

    //
    // common implementation
    template<size_t bytes, size_t align, bool use_meta>
    class PoolAllocatorChain<bytes, align, use_meta, false> final {
    public:
        using AllocationMetaInfo = PoolChainAllocationMetaInfo;
        using AlignInfo = AlignmentInfo<bytes, align>;
        using Allocator = AllocatorType<bytes, align, use_meta>::template Allocator<false>;
        using ChainedPool = PoolChunk<Allocator, uint32_t>;

        // inline const AllocationMetaInfo & getMetaInfo(void * m) const noexcept {
        //     return *AlignInfo::metaInfo(m);
        // }

        [[nodiscard]] inline void * alloc() noexcept {
            for (auto & pool : _poolsChain) {
                if (void * mem = pool->allocator.alloc()) {
                    if constexpr (use_meta) {
                        AlignInfo::metaInfo(mem)->pool = pool;
                    }

                    ++pool->allocations;
                    return mem;
                };
            }

            if (_reservedPool || !_pools.nospace()) {
                auto * pool = _reservedPool ? _poolsChain.emplace_back(_reservedPool) :
                                              _poolsChain.emplace_back(_pools.template create<ChainedPool>(_poolCapacity));
                _reservedPool = nullptr;

                void * mem = pool->allocator.alloc();
                ++pool->allocations;

                if constexpr (use_meta) {
                    AlignInfo::metaInfo(mem)->pool = pool;
                }
                return mem;
            }
            
            assert(false); // not enough memory

            return nullptr;
        }

        inline bool free(void * mem) noexcept {
            if constexpr (use_meta) {
                const auto & info = *AlignInfo::metaInfo(mem);
                auto * pool = static_cast<ChainedPool*>(info.pool);

                if (pool->allocator.free(mem)) {
                    if (--pool->allocations == 0u) {
                        setReservedPool(pool);
                    }
                    return true; 
                }
                assert(false);
                return false;
            }

            // else
            for (size_t i = 0u; i < _pools.capacity(); ++i) {
                auto & pool = _pools.template getValueAs<ChainedPool>(i);
                if (pool.allocator.ownsMemory(mem)) {
                    if (pool.allocator.free(mem)) {
                        if (--pool.allocations == 0u) {
                            setReservedPool(&pool);
                        }
                        return true; 
                    }
                    assert(false);
                }
            }
            return false;
        }

        // object type functions
        template<typename T, typename... Args>
        [[nodiscard]] inline T * create(Args &&... args) { // = alloc with type
            if (auto * m = alloc()) {
                return new(m) T(std::forward<Args>(args)...);
            }
            return nullptr;
        }

        template<typename T>
        inline bool destroy(T * value) noexcept {  // = free with type
            if constexpr (use_meta) {
                const auto & info = *AlignInfo::metaInfo(value);
                auto * pool = static_cast<ChainedPool*>(info.pool);

                if (pool->allocator.destroy(value)) {
                    if (--pool->allocations == 0u) {
                        setReservedPool(pool);
                    }
                    return true; 
                }
                assert(false);
                return false;
            }

            // else
            for (size_t i = 0u; i < _pools.capacity(); ++i) {
                auto & pool = _pools.template getValueAs<ChainedPool>(i);
                if (pool.allocator.ownsMemory(value)) {
                    if (pool.allocator.destroy(value)) {
                        if (--pool.allocations == 0u) {
                            setReservedPool(&pool);
                        }
                        return true; 
                    }
                    assert(false);
                }
            }
            return false;
        }

        inline size_t used_memory() const noexcept { // in bytes
            const size_t staticMemory = sizeof(engine::stl::list<ChainedPool*>) + sizeof(ChainedPool*) +
                                        sizeof(size_t) * 2u + _pools.used_memory();
            const size_t poolsCount = _poolsChain.size();
            const size_t usefulMemory = (poolsCount + (_reservedPool ? 1u : 0u)) * _chunkMemorySize;
            const size_t additionaMemory = poolsCount * sizeof(ChainedPool*);
            return staticMemory + usefulMemory + additionaMemory;
        }

        PoolAllocatorChain(size_t poolCapacity, size_t poolsCount) :
            _poolCapacity(poolCapacity),
            _pools{poolsCount},
            _poolsChain{_pools.template create<ChainedPool>(_poolCapacity)} {
                _chunkMemorySize = _poolsChain.back()->used_memory();
        }

    private:
        inline void setReservedPool(ChainedPool* pool) noexcept {
            if (_reservedPool && _reservedPool != pool) {
                _pools.destroy(_reservedPool);
            }
            _poolsChain.remove(pool);
            _reservedPool = pool;
        }

        size_t _chunkMemorySize = 0u;
        size_t _poolCapacity = 0u;
        PoolAllocator<sizeof(ChainedPool), alignof(ChainedPool), false> _pools;
        engine::stl::list<ChainedPool*> _poolsChain;
        ChainedPool* _reservedPool = nullptr;
    };

    //
    // thread safe implementation
    template<size_t bytes, size_t align, bool use_meta>
    class PoolAllocatorChain<bytes, align, use_meta, true> final {
    public:
        using AllocationMetaInfo = PoolChainAllocationMetaInfo;
        using AlignInfo = AlignmentInfo<bytes, align>;
        using Allocator = AllocatorType<bytes, align, use_meta>::template Allocator<true>;
        using ChainedPool = PoolChunk<Allocator, std::atomic<uint32_t>>;

        // inline const AllocationMetaInfo & getMetaInfo(void * m) const noexcept {
        //     return *AlignInfo::metaInfo(m);
        // }

        [[nodiscard]] inline void * alloc() noexcept {
            __alloc_start:
       
            threads::RWSpinLock::readLock(_rwLock);
            
            for (auto & pool : _poolsChain) {
                if (void * mem = pool->allocator.alloc()) {
                    if constexpr (use_meta) {
                        AlignInfo::metaInfo(mem)->pool = pool;
                    }

                    pool->allocations.fetch_add(1u, std::memory_order_release);

                    //read unlock
                    _rwLock.fetch_sub(1, std::memory_order_release);

                    return mem;
                };
            }

            threads::RWSpinLock::readUnlock(_rwLock);

            {
                const auto chainSize = _poolsChain.size();

                threads::RWSpinLock::writeLock(_rwLock);

                if (chainSize != _poolsChain.size()) {
                    threads::RWSpinLock::writeUnlock(_rwLock);
                    goto __alloc_start; // state changed by other thread
                }

                if (_reservedPool || !_pools.nospace()) {
                    auto * pool = _reservedPool ? _reservedPool : _pools.template create<ChainedPool>(_poolCapacity);
                    _reservedPool = nullptr;
                    _poolsChain.emplace_back(pool);
                    threads::RWSpinLock::writeUnlock(_rwLock);
                    goto __alloc_start;
                }
            }
            
            assert(false); // not enough memory

            return nullptr;
        }

        inline bool free(void * mem) noexcept {
            if constexpr (use_meta) {
                const auto & info = *AlignInfo::metaInfo(mem);
                auto * pool = static_cast<ChainedPool*>(info.pool);

                if (pool->allocator.free(mem)) {
                    if (pool->allocations.fetch_sub(1u, std::memory_order_acq_rel) == 1u) {
                        setReservedPool(pool);
                    }
                    return true; 
                }
                return false;
            }

            // else
            threads::RWSpinLock::readLock(_rwLock);
            for (size_t i = 0u; i < _pools.capacity(); ++i) {
                auto & pool = _pools.template getValueAs<ChainedPool>(i);
                if (pool.allocator.ownsMemory(mem)) {
                    if (pool.allocator.free(mem)) {
                        //read unlock
                        _rwLock.fetch_sub(1, std::memory_order_release);

                        if (pool.allocations.fetch_sub(1u, std::memory_order_acq_rel) == 1u) {
                            setReservedPool(&pool);
                        }
                        return true; 
                    }
                }
            }     
            threads::RWSpinLock::readUnlock(_rwLock);
            return false;
        }

        // object type functions
        template<typename T, typename... Args>
        [[nodiscard]] inline T * create(Args &&... args) { // = alloc with type
            if (auto * m = alloc()) {
                return new(m) T(std::forward<Args>(args)...);
            }
            return nullptr;
        }

        template<typename T>
        inline bool destroy(T * value) noexcept {  // = free with type
            if constexpr (use_meta) {
                const auto & info = *AlignInfo::metaInfo(value);
                auto * pool = static_cast<ChainedPool*>(info.pool);

                if (pool->allocator.destroy(value)) {
                    if (pool->allocations.fetch_sub(1u, std::memory_order_acq_rel) == 1u) {
                        setReservedPool(pool);
                    }
                    return true; 
                }
                return false;
            }

            // else
            threads::RWSpinLock::readLock(_rwLock);
            for (size_t i = 0u; i < _pools.capacity(); ++i) {
                auto & pool = _pools.template getValueAs<ChainedPool>(i);
                if (pool.allocator.ownsMemory(value)) {
                    if (pool.allocator.destroy(value)) {
                        //read unlock
                        _rwLock.fetch_sub(1, std::memory_order_release);

                        if (pool.allocations.fetch_sub(1u, std::memory_order_acq_rel) == 1u) {
                            setReservedPool(&pool);
                        }
                        return true; 
                    }
                }
            }
            threads::RWSpinLock::readUnlock(_rwLock);
            return false;
        }

        PoolAllocatorChain(size_t poolCapacity, size_t poolsCount) :
            _poolCapacity(poolCapacity),
            _pools{poolsCount},
            _poolsChain{_pools.template create<ChainedPool>(_poolCapacity)} {
        }

    private:
        inline void setReservedPool(ChainedPool* pool) noexcept {
            threads::RWSpinLock::writeLock(_rwLock);

            if (pool->allocations.load(std::memory_order_acquire) == 0u) {    
                if (_reservedPool && _reservedPool != pool) {
                    _pools.destroy(_reservedPool);
                }
                _reservedPool = pool;
                _poolsChain.remove(pool);
            }

            threads::RWSpinLock::writeUnlock(_rwLock);
        }

        size_t _poolCapacity = 0u;
        PoolAllocator<sizeof(ChainedPool), alignof(ChainedPool), false> _pools;
        engine::stl::list<ChainedPool*> _poolsChain;
        ChainedPool* _reservedPool = nullptr;
        std::atomic<int32_t> _rwLock = {0};
    };

}