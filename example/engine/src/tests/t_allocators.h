#pragma once

#include "../allocators/pool_allocator_chain.h"
#include "../allocators/stack_allocator.h"
#include "../utils/assert.h"

#include <cstdint>
#include <functional>

namespace engine::tests {
    struct AllocatorsTest {
        void poolAllocator() {
            struct A {
                uint16_t v0;
                double v1;
                std::function<int()> v2;
                A(uint16_t v0, double v1, std::function<int()> && v2) : v0(v0), v1(v1), v2(std::move(v2)) {}
            };

            using AllocatorType = allocators::PoolAllocatorChain<sizeof(A), alignof(A), true, false>;
            using AlignInfo = allocators::AlignmentInfo<sizeof(A), alignof(A)>;

            AllocatorType poolChain(128, 2u);
            constexpr auto s_meta = sizeof(AllocatorType::AllocationMetaInfo);
            constexpr auto s_allocation = AlignInfo::kAllocationSize;
            constexpr auto align_allocation = AlignInfo::kAllocationAlign;

            struct B {
                A v;
                AllocatorType::AllocationMetaInfo info;
            };

            constexpr auto s_allocation2 = sizeof(B);
            constexpr auto align_allocation2 = alignof(B);

            static_assert(s_allocation == s_allocation2);
            static_assert(align_allocation == align_allocation2);

            auto * mem0 = poolChain.alloc();
            auto * mem1 = poolChain.alloc();
            auto * mem2 = poolChain.alloc();
            auto * mem3 = poolChain.alloc();

            poolChain.free(mem0);
            poolChain.free(mem1);
            poolChain.free(mem3);

            auto * mem4 = poolChain.alloc();
            auto * mem5 = poolChain.alloc();
            auto * mem6 = poolChain.alloc();

            ENGINE_BREAK_IF_NOT(mem4 == mem3);
            ENGINE_BREAK_IF_NOT(mem5 == mem1);
            ENGINE_BREAK_IF_NOT(mem6 == mem0);

            auto * val0 = new(mem6) A{10, 20, [](){ return 121; }};
            auto * val1 = new(mem5) A{11, 22, [](){ return 222; }};

            auto * val2 = poolChain.create<A>(12, 23, [](){ return 123; });

            ENGINE_BREAK_IF_NOT(val0->v2() == 121);
            ENGINE_BREAK_IF_NOT(val1->v2() == 222);
            ENGINE_BREAK_IF_NOT(val2->v2() == 123);
        }

        void stackAllocator() {}

        void main() {
            poolAllocator();
            stackAllocator();
        }
    };
}