#pragma once

#include <type_traits>
#include <random>
#include <memory>
#include <cstdint>
#include <ctime>

namespace engine::common {
    
    template <typename T, typename G>
    inline T random(const T min, const T max, G & generator) {
        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> distribution(min, max);
            return distribution(generator);
        }

        if constexpr (std::is_floating_point_v<T>) {
            std::uniform_real_distribution<T> distribution(min, max);
            return distribution(generator);
        }

        return T{};
    }


    template <typename T>
    inline T fixedRandom(const T min, const T max) {
        static thread_local std::mt19937 generator(42u);
        return random(min, max, generator);
    }

    template <typename T>
    inline T random(const T min, const T max) {
        static thread_local std::default_random_engine generator { std::random_device{}() }; // for lightweight randomes
        return random(min, max, generator);
    }

    template <typename T>
    inline T random_HQ(const T min, const T max) {
        static thread_local std::mt19937 generator(std::clock()); // for quality randomnes
        return random(min, max, generator);
    }

}