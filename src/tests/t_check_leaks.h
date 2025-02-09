#pragma once

#include "../threads/thread_pool/thread_pool.h"
#include "../utils/profile/time.h"
#include "../utils/profile/leaks_resolve.h"

namespace engine::tests {
    struct LeaksTest {
        auto main() {
            int * aaa = new int(100);
            int * bbb = new int[10];
            delete[] aaa;
            delete bbb;

            new double();

            for (size_t i = 0; i < 10; ++i) {
                threads::ThreadPool tp(4u, 2048u, 64u);
                int c = 0;
                std::atomic<uint32_t> a_counter = 0;
                engine::profiler::ExecutionTime ex("thread pool ex");
                while (c++ < 100'000) {
                    auto * task = tp.createTask([&a_counter](){
                        new double(222);
                        a_counter.fetch_add(1u, std::memory_order_release);
                        threads::for_each_thread::worker->id();
                        //printf("%d : task, on thread %lu\n", a_counter++, threads::for_each_thread::worker->id());
                    });
                    tp.run(task);
                }
                tp.stop();
                ex.print();
                printf("%d\n", a_counter.load(std::memory_order_acquire));
            }

            return new int();
        }
    };
}