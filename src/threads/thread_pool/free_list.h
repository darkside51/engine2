#pragma once

#include "queue.h"

#include "stl/allocator.h"
#include "stl/vector.h"

#include <mutex>
#include <vector>

namespace threads {
    template <typename T>
    class Freelist {
        struct Head {
            std::mutex mutex;
            UnboundedTaskQueue<T> queue;
        };

    public:
        Freelist(size_t N) : _heads(N) {}

        void push(size_t w, T item) {
            std::scoped_lock lock(_heads[w].mutex);
            _heads[w].queue.push(item);  
        }

        void push(T item) {
            push(reinterpret_cast<uintptr_t>(item) % _heads.size(), item);
        }

        T steal(size_t w) {
            for (size_t i = 0u; i < _heads.size(); i++, w = (w + 1) % _heads.size()) {
                if (auto item = _heads[w].queue.steal(); item) {
                    return item;
                }
            }
            return nullptr;
        }

        bool empty() const noexcept {
            for (const auto & q : _heads) {
                if (!q.queue.empty()) {
                    return false;
                }
            }
            return true;
        }

    private:
        engine::stl::vector<Head, engine::stl::aligned_allocator<alignof(Head)>> _heads;
    };    
}
