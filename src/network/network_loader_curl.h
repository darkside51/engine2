#pragma once

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
    // threads::StopSource2 stop;
    // threads::Worker _worker; // & worker / sh_ptr<Worker> ??
    void * _multiHandle;
    CurlContextsStorage _contextStorage;

    std::vector<std::unique_ptr<RequestTask>> _activeTasks;
    std::queue<std::string> _queue;
};

}