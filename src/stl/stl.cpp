#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

// for eastl can usage with default allocators

extern "C" void EASTL_EASTL_AssertionFailureHandler(const char* pExpression, void* pContext) {
    // Your custom panic, logging, or abort code goes here
    std::abort(); 
}

// void* operator new[](size_t size, const char* pName, int flags, 
//                      unsigned int debugFlags, const char* pFile, int line) {
//     return ::operator new[](size);
// }

// void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, 
//                      const char* pName, int flags, unsigned int debugFlags, 
//                      const char* pFile, int line) {
//     // Basic fallback handling alignment via standard array new
//     return ::operator new[](size);
// }

#ifndef CHECK_MEMORY_LEAKS

#if defined(_MSC_VER)

void* operator new[](size_t size) {
    void* memory = _aligned_malloc(size == 0u ? 1u : size, alignof(std::max_align_t));
    if (memory == nullptr) {
        throw std::bad_alloc();
    }
    return memory;
}

void* operator new[](size_t size, const char* pName, int flags,
                     unsigned int debugFlags, const char* pFile, int line) {
    return _aligned_malloc(size, 16);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, 
                     const char* pName, int flags, unsigned int debugFlags, 
                     const char* pFile, int line) {
    return _aligned_offset_malloc(size, alignment, alignmentOffset);
}

void operator delete[](void* p) noexcept {
    _aligned_free(p);
}

void operator delete[](void* p, size_t size) noexcept {
    _aligned_free(p);
}
#else
// #include <new>

void* operator new[](size_t size, const char* pName, int flags, 
                     unsigned int debugFlags, const char* pFile, int line) {
    return ::operator new[](size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, 
                     const char* pName, int flags, unsigned int debugFlags, 
                     const char* pFile, int line) {
    // posix_memalign requires that alignment be a power of two and no less than sizeof(void*)
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }

    void* ptr = nullptr;
    // if alignmentOffset != 0, a more complex wrapper would be required,
    // but for 99% of EASTL containers, alignmentOffset is 0
    if (posix_memalign(&ptr, alignment, size) != 0) {
        //throw std::bad_alloc();
        return nullptr;
    }
    return ptr;
}
#endif

#endif