#include "network_loader_curl.h"

#include <curl/curl.h>

#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace engine::network {
    
constexpr uint64_t kMaxParallelConnections = 10u;

// reusing curl contexts is recommended practice
class CurlContextsStorage {
public:
    ~CurlContextsStorage() {
        for (auto * context : _contexts) {
            curl_easy_cleanup(context);
        }
        _contexts.clear();
    }
    
    CURL * receiveContext() {
        while (!_contexts.empty()) {
            auto * context = _contexts.front();
            _contexts.pop_front();

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
    
    void returnContext (CURL * curl) {
        assert(curl != nullptr);
        curl_easy_reset(curl);
        _contexts.push_back(curl);
    }

private:
    std::list<CURL*> _contexts;
};

// Структура для управления контекстом каждого запроса
struct RequestTask {
    CurlContextsStorage & contexts;
    CURL * easyHandle = nullptr;
    std::string url;
    std::string response;

    RequestTask(CurlContextsStorage & contexts, std::string_view targetUrl) : 
        contexts(contexts),
        easyHandle(contexts.receiveContext()),
        url(targetUrl) {}
    
    ~RequestTask() {
        contexts.returnContext(easyHandle);
    }
};

// Callback для записи данных
size_t WriteCallback(void * contents, size_t size, size_t nmemb, void * userp) {
    const size_t totalSize = size * nmemb;
    auto * response = static_cast<std::string *>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Функция для создания и настройки нового easy-дескриптора
bool addNewUrlToPool(
    CURLM * multi_handle, 
    const std::string & url, 
    CurlContextsStorage & contexts, 
    std::vector<std::unique_ptr<RequestTask>> & active_tasks
) {
    // Создаем задачу в куче (heap), чтобы ее адрес в памяти не менялся
    auto task = std::make_unique<RequestTask>(contexts, url);
    if (!task->easyHandle) {
        return false;
    }

    curl_easy_setopt(task->easyHandle, CURLOPT_URL, task->url.c_str());
    curl_easy_setopt(task->easyHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(task->easyHandle, CURLOPT_WRITEDATA, &(task->response));
    
    // Передаем указатель на саму структуру task, чтобы найти её при завершении
    curl_easy_setopt(task->easyHandle, CURLOPT_PRIVATE, task.get());
    curl_easy_setopt(task->easyHandle, CURLOPT_TIMEOUT, 5L);

    // Добавляем в multi-пакет прямо во время работы цикла
    curl_multi_add_handle(multi_handle, task->easyHandle);
    
    // Сохраняем умный указатель в векторе, чтобы контролировать время жизни объекта
    active_tasks.push_back(std::move(task));
    return true;
}

}