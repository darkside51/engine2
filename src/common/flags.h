#pragma once

#include <utility>
#include <type_traits>

namespace engine {

    template <typename T> requires std::is_enum_v<T>
    class Flags {
    public:
        Flags() = default;
        Flags(T value) : _value(value) {}
        operator T() const noexcept { return _value; }
        T operator*() const noexcept { return _value; }
        explicit operator bool() const noexcept { return std::to_underlying(_value) != 0; }
        T operator~() const noexcept { return static_cast<T>(~std::to_underlying(_value)); }

        void enable(Flags other) noexcept {
            _value = static_cast<T>(std::to_underlying(_value) | std::to_underlying(other._value));
        }

        void disable(Flags other) noexcept {
            _value = static_cast<T>(std::to_underlying(_value) & ~std::to_underlying(other._value));
        }

        void enable(T value) noexcept {
            _value = static_cast<T>(std::to_underlying(_value) | std::to_underlying(value));
        }

        void disable(T value) noexcept {
            _value = static_cast<T>(std::to_underlying(_value) & ~std::to_underlying(value));
        }

    private:
        T _value = T{0};
    };

    template <typename T>
    inline constexpr Flags<T> operator | (Flags<T> a, Flags<T> b) noexcept {
        return static_cast<T>(std::to_underlying(*a) | std::to_underlying(*b));
    }

    template <typename T>
    inline constexpr Flags<T> operator | (Flags<T> a, T b) noexcept {
        return static_cast<T>(std::to_underlying(*a) | std::to_underlying(b));
    }

    template <typename T>
    inline constexpr Flags<T> operator & (Flags<T> a, Flags<T> b) noexcept {
        return static_cast<T>(std::to_underlying(*a) & std::to_underlying(*b));
    }

    template <typename T>
    inline constexpr Flags<T> operator & (Flags<T> a, T b) noexcept {
        return static_cast<T>(std::to_underlying(*a) & std::to_underlying(b));
    }

}