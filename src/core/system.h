#pragma once

#include <cstdint>

inline auto constexpr isLittleEndian = []() noexcept {
	constexpr uint32_t i = 0x01234567u;
	// return 0 for big endian, 1 for little endian.
	return (*((uint8_t*)(&i))) == 0x67u;
};

//-----------------------------------------------------------------------------
// Cache line alignment
//-----------------------------------------------------------------------------
#if defined(__i386__) || defined(__x86_64__)
constexpr uint32_t kCachelineSize = 64u;
#elif defined(__powerpc64__)
  // This is the L1 D-cache line size of our Power7 machines.
  // Need to check if this is appropriate for other PowerPC64 systems.
  constexpr uint32_t kCachelineSize = 128u;
#elif defined(__arm__)
  // Cache line sizes for ARM: These values are not strictly correct since
  // cache line sizes depend on implementations, not architectures.
  // There are even implementations with cache line sizes configurable
  // at boot time.
  #if defined(__ARM_ARCH_5T__)
    constexpr uint32_t kCachelineSize = 32u;
  #elif defined(__ARM_ARCH_7A__)
    constexpr uint32_t kCachelineSize = 64u;
  #else // ARM v8
    constexpr uint32_t kCachelineSize = 64u;
  #endif
#else
// A reasonable default guess.  Note that overestimates tend to waste more
// space, while underestimates tend to waste more time.
constexpr uint32_t kCachelineSize = 64u;
#endif

namespace engine {
    
}