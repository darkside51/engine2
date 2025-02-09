#pragma once

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