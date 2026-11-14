#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

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

enum class MemoryPrefetchMode : uint8_t { // specifies the nature of future memory access
	  eRead = 0u, // preparation for reading data
	  eWrite = 1u, // preparation for a memory write (helps avoid unnecessary cache-line transfers in multiprocessor systems)
	  eGeneral = 2u // preparation for a shared read (data will be read by other cores before being overwritten; supported in recent versions of GCC)
};

enum class MemoryLocality : uint8_t { // temporal locality — how long data should be kept in the cache after being used
	  eOnce = 0u, // the data is needed for a one-time use (do not keep it in the cache; minimize cache pollution).
	  eLow = 1u, // low locality (load only into L3/L2 cache)
	  eModerate = 2u, // moderate locality
    eHigh = 3u // high locality (loading into all cache levels, including the fast L1)
};

template <MemoryPrefetchMode Mode = MemoryPrefetchMode::eRead,
          MemoryLocality Locality = MemoryLocality::eHigh>
inline void prefetchMemory(const void * ptr) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    if (ptr != nullptr) {
        __builtin_prefetch(ptr, static_cast<uint8_t>(Mode), static_cast<uint8_t>(Locality));
    }
#else
    static_cast<void>(ptr);
#endif
}

}