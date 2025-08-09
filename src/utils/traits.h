#pragma once

#include <tuple>
#include <type_traits>

namespace engine::traits {

    template <typename... Ts>
    struct Types {};

    template <class T, class Tuple>
    struct TypeIndex;

    template <class T, class... Ts>
    struct TypeIndex<T, std::tuple<T, Ts...>> {
        static constexpr std::size_t value = 0u;
    };

    template <class T, class U, class... Ts>
    struct TypeIndex<T, std::tuple<U, Ts...>> {
        static constexpr std::size_t value = 1u + TypeIndex<T, std::tuple<Ts...>>::value;
    };

    template <typename...>
    struct Tuple;

    template <typename... Ts>
    struct Tuple<Types<Ts...>> : std::tuple<Ts...> {
        using Base_Type = std::tuple<Ts...>;

        template <typename... Us>
        Tuple(Us&&... other) : std::tuple<Ts...>(std::forward<Us>(other)...) {};

        template<typename T>
        T& get() noexcept { return std::get<T>(*this); }

        template<typename T>
        const T& get() const noexcept { return std::get<T>(*this); }

        template<size_t index>
        auto& get() noexcept { return std::get<index>(*this); }

        template<size_t index>
        const auto& get() const noexcept { return std::get<index>(*this); }
    };
}