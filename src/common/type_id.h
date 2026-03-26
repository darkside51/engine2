#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace engine {
    namespace run_time_type_id {
        inline static std::atomic_size_t gStaticId = 0u;

        template<typename T>
        inline size_t typeIndex() noexcept {
            static const size_t newId = gStaticId.fetch_add(1u, std::memory_order_relaxed);
            return newId;
        }

        template<typename MainType>
        struct UniqueTypeId {
            static inline size_t typeId() noexcept {
                return typeIndex<MainType>();
            }

            template<typename T>
            static inline size_t typeId() noexcept {
                static const size_t newId = staticId.fetch_add(1u, std::memory_order_relaxed);
                return newId;
            }
        private:
            inline static std::atomic_size_t staticId = 0u;
        };
    }

    namespace compile_time_type_id {
        template<typename Type, auto id>
        struct TypeCounter {
        private:
            struct Generator {
                friend consteval bool isDefined(TypeCounter) noexcept { return true; }
            };

            friend consteval bool isDefined(TypeCounter) noexcept;

        public:
            template<typename T = TypeCounter, auto = isDefined(T{})>
            static consteval bool exists(decltype(id)) noexcept { return true; }

            static consteval bool exists(...) noexcept { Generator(); return false; }
        };

        template<typename T, auto id = size_t{}>
        consteval auto typeIndex() noexcept {
            if constexpr (TypeCounter<std::nullptr_t, id>::exists(id)) {
                return typeIndex<T, id + 1u>();
            } else {
                return id;
            }
        }

        template<typename T, typename U, auto id = size_t{}>
        consteval auto typeIndex() noexcept {
            if constexpr (TypeCounter<T, id>::exists(id)) {
                return typeIndex<T, U, id + 1u>();
            } else {
                return id;
            }
        }

        template<typename MainType>
        struct UniqueTypeId {
            static consteval inline auto typeId() noexcept {
                return typeIndex<MainType>();
            }

            template<typename T>
            static consteval inline auto typeId() noexcept {
                return typeIndex<MainType, T>();
            }
        };
    }

    namespace rtti = run_time_type_id;
    namespace ctti = compile_time_type_id;
}