#include "network_loader_curl.h"
#include "utils/format_string.h"

#include <curl/curl.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

namespace engine::network {
    
constexpr uint64_t kMaxParallelConnections = 10u;
constexpr uint32_t kBufferSize = 1024u * 1024u;  // 1Mb

CurlContextsStorage::~CurlContextsStorage() {
    for (auto * context : _contexts) {
        curl_easy_cleanup(context);
    }
    _contexts.clear();
}

CURL * CurlContextsStorage::receiveContext() {
    while (!_contexts.empty()) {
        auto * context = _contexts.back();
        _contexts.pop_back();

        long socket;
        const CURLcode code = curl_easy_getinfo(context, CURLINFO_ACTIVESOCKET, &socket);

        if (code == CURLE_OK && socket != CURL_SOCKET_BAD) {
            assert(context != nullptr);
            return context;
        }

        curl_easy_cleanup(context);
    }

    auto * context = curl_easy_init();
    assert(context != nullptr);
    return context;
}

void CurlContextsStorage::returnContext (CURL * curl) {
    assert(curl != nullptr);
    curl_easy_reset(curl);
    _contexts.push_back(curl);
}

// callback для записи данных
size_t writeCallback(void * data, size_t size, size_t nmemb, void * userp) {
    const size_t totalSize = size * nmemb;
    auto * payload = static_cast<CurlNetworkLoader::Payload *>(userp);
    auto const * bytes = reinterpret_cast<std::byte const *>(data);
    payload->response.insert(payload->response.end(), bytes, bytes + totalSize);
    return totalSize;
}

size_t progressCallback(void * userp, curl_off_t dltotal,curl_off_t dlnow, 
    curl_off_t ultotal, curl_off_t ulnow) {
    const auto * payload = static_cast<const CurlNetworkLoader::Payload *>(userp);
    if (payload->stopToken.stop_requested()) {
        return 1; // cancel
    }
    return 0;
}

CurlNetworkLoader::CurlNetworkLoader() {
    curl_global_init(CURL_GLOBAL_ALL);
    _multiHandle = curl_multi_init();
    curl_multi_setopt(_multiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, kMaxParallelConnections);

    _executor = std::unique_ptr<threads::ExecutionContext>{
        new threads::Executor([&](auto & executionContext, threads::ExecutionTimePoint now, float delta) {
            executionContext.performTasks(0u);
            process();
    }, 0.05f, true)};
}

CurlNetworkLoader::~CurlNetworkLoader() {
    _executor->stop();
    curl_multi_cleanup(_multiHandle);
    curl_global_cleanup();
}

void CurlNetworkLoader::process() {
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
            void * completedTask = nullptr;
            curl_easy_getinfo(handle, CURLINFO_PRIVATE, &completedTask);

            if (completedTask) {
                auto * payload = static_cast<Payload*>(completedTask);
                if (payload->stopToken.stop_requested()) {
                    std::unique_ptr<Payload>{payload};
                } else {
                    long response_code = 0;
                    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);

                    if (msg->data.result == CURLE_OK) {
                        printf("download complete!!!!\n");
                    }

                    std::move(payload->callback)(payload, std::move(payload->response));
                }

// HTTP Status Codes (From the Server)
// These are standard three-digit codes sent by a web server indicating if a request succeeded or failed. 
// curl does not treat 4xx or 5xx server errors as command failures unless you explicitly use the -f / --fail flag.
// 1xx (Informational): Request received, continuing process.
// 2xx (Success): The action was successfully received and accepted (e.g., 200 OK).
// 3xx (Redirection): Further action needs to be taken to complete the request (e.g., 301 or 302 redirects).
// 4xx (Client Error): The request contains bad syntax or cannot be fulfilled (e.g., 404 Not Found, 401 Unauthorized).
// 5xx (Server Error): The server failed to fulfill an apparently valid request (e.g., 500 Internal Server Error).

                //printf("url: %s response code: %ld\n", completed_task->url.c_str(), response_code);
            }

            // Удаляем отработавший дескриптор из multi-обработки
            curl_multi_remove_handle(_multiHandle, handle);
            _contextStorage.returnContext(handle);
        }
    }
}

void CurlNetworkLoader::setupHandle(std::unique_ptr<Payload> && payloadPtr, http::Request && request) {
    auto handle = _contextStorage.receiveContext();
    if (!handle) {
        return;
    }

    auto * payload = payloadPtr.release();
    
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
    
    if (payload->stopToken.stop_requested()) {
        _contextStorage.returnContext(handle);
        std::unique_ptr<Payload>{payload};
        return;
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

    // Добавляем в multi-пакет прямо во время работы цикла
    curl_multi_add_handle(_multiHandle, handle);
}

}