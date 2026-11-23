#pragma once

#if not  defined(_MSC_VER)

#ifdef CHECK_MEMORY_LEAKS

namespace engine::leaks {
    inline class MemoryLeaksChecker {
    public:
        MemoryLeaksChecker();
        ~MemoryLeaksChecker() noexcept;
    private:
    } leaksChecker;
}

#endif

#endif