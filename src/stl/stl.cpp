#include <cstddef>

// for eastl can usage with default allocators

void* operator new[](size_t size, const char* pName, int flags, 
                     unsigned int debugFlags, const char* pFile, int line) {
    return ::operator new[](size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, 
                     const char* pName, int flags, unsigned int debugFlags, 
                     const char* pFile, int line) {
    // Basic fallback handling alignment via standard array new
    return ::operator new[](size);
}