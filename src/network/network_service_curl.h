#pragma once

#include "allocators/pool_allocator.h"
#include "http.h"
#include "threads/executor.h"

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace engine::network {

// reusing curl contexts is recommended practice
class CurlContextsStorage2 {
public:
    CurlContextsStorage2(uint16_t size);
    ~CurlContextsStorage2();
    void * receiveContext();
    void returnContext (void * curl);

private:
    std::vector<void*> _contexts;
    uint16_t _current = 0u;
};
    
class CurlNetworkService {
public:
    struct Payload {
        threads::StopToken stopToken;
        std::vector<std::byte> response;
        std::move_only_function<void(Payload *, std::vector<std::byte> &&)> callback;
    };

    CurlNetworkService(size_t payloadPoolCapacity = 256u);
    ~CurlNetworkService();

    template <typename T>
    void sendRequest(http::Request && request, T && callback, threads::StopToken && stopToken) {
        _executor->emplaceTask({[this, request = std::move(request), callback = std::move(callback), stopToken = std::move(stopToken)]() mutable {
            if (stopToken.stop_requested()) {
                return;
            }

            auto payload = _payloadAllocator.create<Payload>(std::move(stopToken));
            payload->callback = [this, callback = std::move(callback)](Payload * payload, std::vector<std::byte> && response) {
                callback(std::move(response));
                _payloadAllocator.destroy(payload);
            };

            setupHandle(std::move(payload), std::move(request));
        }}, 0u);
    }

private:
    using Allocator = allocators::PoolAllocator<sizeof(Payload), alignof(Payload), false>;
    Allocator _payloadAllocator;

    void setupHandle(Payload * payload, http::Request && request);
    void process();

    std::unique_ptr<threads::ExecutionContext> _executor;

    void * _multiHandle = nullptr;
    CurlContextsStorage2 _contextStorage;

    uint32_t _speedLimitTimeout = 20u; // 20 seconds
};

}