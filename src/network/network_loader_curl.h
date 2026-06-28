#pragma once

#include "threads/executor.h"

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
    
struct RequestTask;

class CurlNetworkLoader {
public:
    CurlNetworkLoader();
    ~CurlNetworkLoader();

    void sendRequest(std::string_view url);

private:
    void process();
    // threads::StopSource2 stop;
    std::unique_ptr<threads::ExecutionContext> _executor;

    void * _multiHandle;
    CurlContextsStorage _contextStorage;

    std::vector<std::unique_ptr<RequestTask>> _activeTasks;
};

}