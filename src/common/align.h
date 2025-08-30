#pragma once

#include <cstdint>
#include <functional>
#include <type_traits>
		
namespace engine {
    template <typename T, typename U = T>
	constexpr inline std::decay_t<T> align_value(const T value, const U align) noexcept {
		return (value + align - 1) & ~(align - 1);
	}
}
