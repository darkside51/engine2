#include "network_service_curl.h"
#include "utils/format_string.h"
#include "log/log.h"

#include <curl/curl.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace engine::network {
    
constexpr uint64_t kMaxParallelConnections = 10u;
constexpr uint32_t kBufferSize = 1024u * 1024u; // 1Mb
constexpr auto kMmilliseconds = std::chrono::milliseconds{50};
constexpr float kExecutionTimeDeltaSeconds = std::chrono::duration<float>(kMmilliseconds).count();

CurlContextsStorage2::CurlContextsStorage2(uint16_t size) : _contexts(size) {}
CurlContextsStorage2::~CurlContextsStorage2() {
    for (auto * context : _contexts) {
        curl_easy_cleanup(context);
    }
    _contexts.clear();
}

void * CurlContextsStorage2::receiveContext() {
    auto *& context = _contexts[_current];
    ++_current;
    if (context) {
        long socket;
        const CURLcode code = curl_easy_getinfo(context, CURLINFO_ACTIVESOCKET, &socket);

        if (code == CURLE_OK && socket != CURL_SOCKET_BAD) {
            assert(context != nullptr);
            return context;
        }

        curl_easy_cleanup(context);
    }
    context = curl_easy_init();
    assert(context != nullptr);
    return context;
}

void CurlContextsStorage2::returnContext (void * curl) {
    assert(curl != nullptr);
    curl_easy_reset(curl);
    _contexts[--_current] = curl;
}

size_t writeCallback(void * data, size_t size, size_t nmemb, void * userp) {
    const size_t totalSize = size * nmemb;
    auto * payload = static_cast<CurlNetworkService::Payload *>(userp);
    auto const * bytes = reinterpret_cast<std::byte const *>(data);
    payload->response.insert(payload->response.end(), bytes, bytes + totalSize);
    return totalSize;
}

size_t progressCallback(void * userp, curl_off_t dltotal,curl_off_t dlnow, 
    curl_off_t ultotal, curl_off_t ulnow) {
    const auto * payload = static_cast<const CurlNetworkService::Payload *>(userp);
    if (payload->stopToken.stop_requested()) {
        return 1u; // cancel
    }
    return 0u;
}

CurlNetworkService::CurlNetworkService(size_t payloadPoolCapacity) : 
    _payloadAllocator{payloadPoolCapacity},
    _contextStorage(payloadPoolCapacity) {

    curl_global_init(CURL_GLOBAL_ALL);
    _multiHandle = curl_multi_init();
    curl_multi_setopt(_multiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, kMaxParallelConnections);

    _executor = std::unique_ptr<threads::ExecutionContext>{
        new threads::Executor([&](auto & executionContext, threads::ExecutionTimePoint now, float delta) {
            executionContext.performTasks(0u);
            process();
    }, kExecutionTimeDeltaSeconds, true)};
}

CurlNetworkService::~CurlNetworkService() {
    _executor->stop();
    curl_multi_cleanup(_multiHandle);
    curl_global_cleanup();
}

void CurlNetworkService::process() {
    // 2. Делаем шаг сетевого обмена данных
    int still_running = 0;
    CURLMcode mc = curl_multi_perform(_multiHandle, &still_running);
    if (mc != CURLM_OK) {
        return;
    }

    // 3. Проверяем, завершился ли какой-нибудь запрос
    int msgs_left;
    CURLMsg * msg;
    while ((msg = curl_multi_info_read(_multiHandle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL * handle = msg->easy_handle;
            
            // Получаем контекст нашей задачи через CURLOPT_PRIVATE
            void * request = nullptr;
            curl_easy_getinfo(handle, CURLINFO_PRIVATE, &request);

            if (request) {
                auto * payload = static_cast<Payload*>(request);
                if (payload->stopToken.stop_requested()) {
                    std::move(payload->callback)(payload, {}, RequestResult::eCancelled);
                    _payloadAllocator.destroy(payload);
                } else {
                    if (msg->data.result == CURLE_OK) {
                        long response_code = 0;
                        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
                        if (response_code == 200) {
                            std::move(payload->callback)(payload, std::move(payload->response), RequestResult::eSuccess);
                        } else {
                            std::move(payload->callback)(payload, std::move(payload->response), RequestResult::eHttpError);
                        }
                    } else if (msg->data.result == CURLE_HTTP_RETURNED_ERROR) {
                        long response_code = 0;
                        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
                        LOG_E("request: %s, curl http error, code: %ld", payload->url.c_str(), response_code);
                        std::move(payload->callback)(payload, {}, RequestResult::eHttpError);
                    } else {
                        LOG_E("request: %s, curl error, result: %s", payload->url.c_str(), curl_easy_strerror(msg->data.result));
                        std::move(payload->callback)(payload, {}, RequestResult::eUndefined);
                    }
                }
            }

            // Удаляем отработавший дескриптор из multi-обработки
            curl_multi_remove_handle(_multiHandle, handle);
            _contextStorage.returnContext(handle);
        } else {
            LOG_E("curl error, message: %d", std::to_underlying(msg->msg));
            return;
        }
    }
}

void CurlNetworkService::setupHandle(Payload * payload, http::Request && request) {
    auto handle = _contextStorage.receiveContext();
    if (!handle) {
        _payloadAllocator.destroy(payload);
        return;
    }

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, payload);

    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0); // for use progress callback
    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, progressCallback);
    curl_easy_setopt(handle, CURLOPT_XFERINFODATA, payload);
    
    // Передаем указатель на саму структуру payload, чтобы найти её при завершении
    curl_easy_setopt(handle, CURLOPT_PRIVATE, payload);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, request.timeout);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, request.connectTimeout);
    curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, _speedLimitTimeout);

    curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, kBufferSize);

    // Disable the Nagle algorithm (Send packets immediately)
    curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 1);

    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 0);

    //curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);

    // enable redirections
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);

    if (!request.proxy.empty()) {
        curl_easy_setopt(handle, CURLOPT_PROXY, request.proxy.c_str());
    }

    // SSL/TLS-сертификат
    if (!request.certificate.empty()) {
        curl_easy_setopt(handle, CURLOPT_CAINFO, request.certificate.c_str());
    }
    
    switch (request.method) {
        case http::Method::eGet: 
        {
            curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
            if (request.str.empty()) {
                curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str());
            } else {
                curl_easy_setopt(handle, CURLOPT_URL, fmt_string("%s?%s", request.url.c_str(), request.str.c_str()));
            }
        }
        break;
        case http::Method::ePost:
        {
            curl_easy_setopt(handle, CURLOPT_POST, 1);
            curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, request.str.size());
            curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, request.str.c_str());
            curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str());
        }
        break;
        default: // unsupported method!
            assert(false);
            break;
    }

     if (payload->stopToken.stop_requested()) {
        _contextStorage.returnContext(handle);
        _payloadAllocator.destroy(payload);
        return;
    }
    
    // Добавляем в multi-пакет прямо во время работы цикла
    curl_multi_add_handle(_multiHandle, handle);
}

}