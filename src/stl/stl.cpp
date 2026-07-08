#include <cstddef>
#include <cstdlib>

// for eastl can usage with default allocators

extern "C" void EASTL_EASTL_AssertionFailureHandler(const char* pExpression, void* pContext) {
    // Your custom panic, logging, or abort code goes here
    std::abort(); 
}

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