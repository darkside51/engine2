#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
		
namespace engine {

	template <typename T, typename U>
	inline size_t offset_of(U T::*member_ptr) noexcept {
		return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*member_ptr));
	}

    template <typename T, typename U = T>
	constexpr inline std::decay_t<T> align_value(const T value, const U align) noexcept {
		return (value + align - 1) & ~(align - 1);
	}

	template <typename T, typename U = T>
	constexpr inline std::decay_t<T> align_value_no_pow2(const T value, const U align) noexcept {
		return ((value + align - 1) / align) * align;
	}
}
