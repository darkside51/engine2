#pragma once

#include <cstdint>
#include <string>

namespace engine::network::http {

enum class Method : uint8_t {
    eGet = 0u,
    ePost = 1u
}; 

struct Request {
    std::string url;
    std::string str;
    std::string proxy;
    std::string certificate;
    uint64_t timeout = 5000u;
    uint64_t connectTimeout = 1000u;
    Method method = Method::eGet;
};

}