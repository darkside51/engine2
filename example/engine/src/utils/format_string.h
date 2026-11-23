#pragma once

#include <cstdint>
#include <cstdio>

namespace engine {
    template <typename...Args>
	inline const char * fmt_string(const char * format, Args&&...args) noexcept {
		constexpr uint16_t kBufferSize = 1024u;
		static thread_local char buffer[kBufferSize]; // kBufferSize bytes memory for every thread, with static allocation
        snprintf(buffer, kBufferSize, format, std::forward<Args>(args)...);
		return buffer;
	}
}