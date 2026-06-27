#include "network_loader_curl.h"

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

CurlContextsStorage::~CurlContextsStorage() {
    for (auto * context : _contexts) {
        curl_easy_cleanup(context);
    }
    _contexts.clear();
}

CURL * CurlContextsStorage::receiveContext() {
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

void CurlContextsStorage::returnContext (CURL * curl) {
    assert(curl != nullptr);
    curl_easy_reset(curl);
    _contexts.push_back(curl);
}


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
    CURLM * multiHandle, 
    std::string_view url,
    uint64_t timeout, // 5000L
    CurlContextsStorage & contexts, 
    std::vector<std::unique_ptr<RequestTask>> & activeTasks
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
    curl_easy_setopt(task->easyHandle, CURLOPT_TIMEOUT_MS, timeout);

    // Добавляем в multi-пакет прямо во время работы цикла
    curl_multi_add_handle(multiHandle, task->easyHandle);
    
    // Сохраняем умный указатель в векторе, чтобы контролировать время жизни объекта
    activeTasks.push_back(std::move(task));
    return true;
}

void process(
    CurlContextsStorage & contexts, 
    CURLM * multiHandle, 
    std::queue<std::string> & discoveryQueue, 
    std::vector<std::unique_ptr<RequestTask>> & activeTasks) {
    int still_running = 0;
    bool initial_load = true;

    // Цикл работает, пока cURL что-то качает ИЛИ пока у нас есть новые нераспарсенные ссылки в очереди
    while (still_running > 0 || !discoveryQueue.empty() || initial_load) {
        initial_load = false;

        // 1. Извлекаем ссылки из очереди и сразу же бросаем их в работу cURL
        while (!discoveryQueue.empty()) {
            std::string nextUrl = std::move(discoveryQueue.front());
            discoveryQueue.pop();
            
            //std::cout << "[Очередь] Добавляем новый URL: " << next_url << std::endl;
            addNewUrlToPool(multiHandle, nextUrl, 5000L, contexts, activeTasks);
        }

        // 2. Делаем шаг сетевого обмена данных
        CURLMcode mc = curl_multi_perform(multiHandle, &still_running);
        if (mc != CURLM_OK) break;

        // 3. Проверяем, завершился ли какой-нибудь запрос
        int msgs_left;
        CURLMsg * msg;
        while ((msg = curl_multi_info_read(multiHandle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL * easy_handle = msg->easy_handle;
                
                // Получаем контекст нашей задачи через CURLOPT_PRIVATE
                RequestTask * completed_task = nullptr;
                curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &completed_task);

                if (completed_task) {
                    long response_code = 0;
                    curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &response_code);

                    if (msg->data.result == CURLE_OK) {
                        printf("download complete!!!!\n");
                    }

// HTTP Status Codes (From the Server)
// These are standard three-digit codes sent by a web server indicating if a request succeeded or failed. 
// curl does not treat 4xx or 5xx server errors as command failures unless you explicitly use the -f / --fail flag.
// 1xx (Informational): Request received, continuing process.
// 2xx (Success): The action was successfully received and accepted (e.g., 200 OK).
// 3xx (Redirection): Further action needs to be taken to complete the request (e.g., 301 or 302 redirects).
// 4xx (Client Error): The request contains bad syntax or cannot be fulfilled (e.g., 404 Not Found, 401 Unauthorized).
// 5xx (Server Error): The server failed to fulfill an apparently valid request (e.g., 500 Internal Server Error).

                    printf("url: %s response code: %ld\n", completed_task->url.c_str(), response_code);
                    // std::cout << "[Готово] Скачан: " << completed_task->url << " (HTTP " << response_code << ")" << std::endl;

                    // // --- ИМИТАЦИЯ ПАРСИНГА СТРАНИЦЫ ---
                    // // В реальном коде здесь должна быть библиотека вроде Gumbo HTML или regex.
                    // // Если мы скачали стартовую страницу, "находим" на ней еще две новые ссылки.
                    // if (completed_task->url == "https://httpbin.org" && msg->data.result == CURLE_OK) {
                    //     std::cout << "[Парсер] Найдена ссылка -> page=sub_link_A" << std::endl;
                    //     discovery_queue.push("https://httpbin.org");
                        
                    //     std::cout << "[Парсер] Найдена ссылка -> page=sub_link_B" << std::endl;
                    //     discovery_queue.push("https://httpbin.org");
                    // }
                    // ----------------------------------
                }

                // Удаляем отработавший дескриптор из multi-обработки
                curl_multi_remove_handle(multiHandle, easy_handle);
                
                // Удаляем задачу из вектора active_tasks, чтобы освободить оперативную память
                for (auto it = activeTasks.begin(); it != activeTasks.end(); ++it) {
                    if (it->get() == completed_task) {
                        activeTasks.erase(it);
                        break; 
                    }
                }
            }
        }

        // 4. Спим, ожидая сетевых событий.
        // ВАЖНО: Ставим маленький таймаут (например, 50-100мс), чтобы цикл быстро возвращался 
        // к проверке discovery_queue, если сеть временно молчит, но появились локальные задачи.
        if (still_running > 0) {
            curl_multi_poll(multiHandle, NULL, 0, 50, NULL);
        }
    }
}

CurlNetworkLoader::CurlNetworkLoader() {
    curl_global_init(CURL_GLOBAL_ALL);
    _multiHandle = curl_multi_init();
    curl_multi_setopt(_multiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, kMaxParallelConnections);  
}

CurlNetworkLoader::~CurlNetworkLoader() {
    curl_multi_cleanup(_multiHandle);
    curl_global_cleanup();
}

void CurlNetworkLoader::sendRequest(std::string_view url) {
    _queue.emplace(url);
    process(_contextStorage, _multiHandle, _queue, _activeTasks);
}

}