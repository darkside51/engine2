#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

namespace threads {

template <typename T>
class unique_t {
public:
    inline static uint8_t id() noexcept {
        const static thread_local uint8_t id = nextId.fetch_add(1u, std::memory_order_relaxed);
        return id;
    }

private:
    inline static std::atomic_uint8_t nextId = {0u};
};

using unique = unique_t<void>;

}