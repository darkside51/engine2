#pragma once

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
class CurlContextsStorage {
public:
    ~CurlContextsStorage();
    void * receiveContext();
    void returnContext (void * curl);

private:
    std::list<void*> _contexts;
};
    
class CurlNetworkLoader {
public:
    struct Payload {
        threads::StopToken stopToken;
        std::vector<std::byte> response;
        std::move_only_function<void(Payload *, std::vector<std::byte> &&)> callback;
    };

    CurlNetworkLoader();
    ~CurlNetworkLoader();

    template <typename T>
    void sendRequest(http::Request && request, T && callback, threads::StopToken && stopToken) {
        _executor->emplaceTask({[this, request = std::move(request), callback = std::move(callback), stopToken = std::move(stopToken)]() mutable {
            if (stopToken.stop_requested()) {
                return;
            }

            auto payload = std::make_unique<Payload>(std::move(stopToken));
            payload->callback = [callback = std::move(callback)](Payload * payload, std::vector<std::byte> && response) {
                std::unique_ptr<Payload>{payload};
                callback(std::move(response));
            };

            setupHandle(std::move(payload), std::move(request));
        }}, 0u);
    }

private:
    void setupHandle(std::unique_ptr<Payload> && payloadPtr, http::Request && request);
    void process();

    std::unique_ptr<threads::ExecutionContext> _executor;

    void * _multiHandle;
    CurlContextsStorage _contextStorage;

    uint32_t _speedLimitTimeout = 100; // 10 seconds
};

}