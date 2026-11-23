#pragma once

#include "utils/traits.h"

#include <cstdint>

namespace engine::memory {
    static constexpr uint8_t tagShift = 57u; // 57 bits
    static constexpr uint8_t tagBits = 64u - tagShift;
    static constexpr uint64_t tagMask = ((1ull << tagBits) - 1u) << tagShift;
    static constexpr uint64_t ptrMask = ~tagMask;

    template <typename T = void>
    inline constexpr T* ptr_tag(const void * ptr, uint8_t tag) noexcept {
        return reinterpret_cast<T*>(reinterpret_cast<uint64_t>(ptr) | (static_cast<uint64_t>(tag) << tagShift));
    }

    inline constexpr uint8_t extract_tag(const void * ptr) noexcept {
        return static_cast<uint8_t>(reinterpret_cast<uint64_t>(ptr) >> tagShift);
    }

    template <typename T = void>
    inline constexpr T* extract_ptr(const void * ptr) noexcept {
        return reinterpret_cast<T*>(reinterpret_cast<uint64_t>(ptr) & ptrMask);
    }

    // usage:
    // int a = 124;
    // const uint8_t b = 11u;
    // const auto * ptr1 = ptr_tag(&a, b);
    // const auto vb = extract_tag(ptr1);
    // const auto va = *extract_ptr<int>(ptr1);

    template <typename... Ts>
    class tag_ptr {
        using types = std::tuple<Ts...>;
        constexpr static std::tuple_element_t<0, types>* kNull_none = nullptr;

    public:
        tag_ptr() = default;
        tag_ptr(nullptr_t) : _ptr(nullptr) {}

        template<typename T>
        tag_ptr(T * ptr) : _ptr(ptr_tag(ptr, traits::TypeIndex<T, types>::value)) {}

        template <typename F, size_t N = 0u, typename... Args>
        auto call(F && f, Args && ... args) noexcept -> decltype(f(kNull_none, std::forward<Args>(args)...)) {
            using RT = decltype(f(kNull_none, std::forward<Args>(args)...));
            if (_ptr == nullptr) {
                if constexpr (std::is_same_v<void, RT>) {
                    return;
                } else {
                    return {};
                }
            }

            if constexpr (N >= std::tuple_size_v<types>) {
                if constexpr (std::is_same_v<void, RT>) {
                    return;
                } else {
                    return {};
                }
            } else {
                const auto tag = extract_tag(_ptr);
                if (tag == N) {
                    using T = std::tuple_element_t<N, types>;
                    return f(extract_ptr<T>(_ptr), std::forward<Args>(args)...);
                }

                return call<F, N + 1u, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
            }
        }
        
    private:
        void * _ptr = nullptr;
    };

    // usage:
    // struct T0 {
    //     int a = 0;
    //     void f() {++a;}
    // };

    // struct T1 {
    //     double a = 100.0;
    //     void f() {a -= 10.0;}
    // };
    // T0 t;
    // T1 t2;

    // tag_ptr<T0, T1> tptr0(&t);
    // tag_ptr<T0, T1> tptr1(&t2);
    // tptr0.call([](auto * ptr){
    //     ptr->f();
    // });
    // auto value = tptr1.call([](auto * ptr, int v) noexcept {
    //     ptr->f();
    //     return ptr->a + v;
    // }, 110);

}
