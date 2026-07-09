#pragma once

#include "EASTL/allocator.h"

namespace eastl {
    template <size_t Alignment>
    class aligned_allocator {
    public:
        aligned_allocator(const char* name = nullptr) { (void)name; }
        aligned_allocator(const aligned_allocator&) {}
        aligned_allocator(const aligned_allocator&, const char*) {}

        aligned_allocator& operator=(const aligned_allocator&) { return *this; }

        bool operator==(const aligned_allocator&) const { return true; }
        bool operator!=(const aligned_allocator&) const { return false; }

        void* allocate(size_t n, int flags = 0)
        {
            (void)flags;

            #if defined(_MSC_VER)
                return _aligned_malloc(n, Alignment);
            #else
                void* ptr = nullptr;
                size_t min_align = (Alignment < sizeof(void*)) ? sizeof(void*) : Alignment;
                if (posix_memalign(&ptr, min_align, n) != 0)
                    return nullptr;
                return ptr;
            #endif
        }

        void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
        {
            // For complex alignments natively driven by the container
            (void)offset; (void)flags;
  
            #if defined(_MSC_VER)
                return _aligned_malloc(n, alignment);
            #else
                void* ptr = nullptr;
                size_t min_align = (alignment < sizeof(void*)) ? sizeof(void*) : alignment;
                if (posix_memalign(&ptr, min_align, n) != 0)
                    return nullptr;
                return ptr;
            #endif
        }

        void deallocate(void* p, size_t n)
        {
            (void)n;
            #if defined(_MSC_VER)
                _aligned_free(p);
            #else
                free(p);
            #endif
        }
    };
}

namespace engine::stl {
    using allocator = eastl::allocator;

    template<size_t A>
    using aligned_allocator = eastl::aligned_allocator<A>;
}