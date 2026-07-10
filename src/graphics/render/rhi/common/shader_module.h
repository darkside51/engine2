#pragma once

#include "common/hash.h"
#include "stl/unordered_map.h"

#include "runtime.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::rhi {

template<typename Api>
class ShaderModulesManager final : public runtime::ShaderModulesManager {
    using ModulesMap = stl::unordered_map<std::string, typename Api::ShaderModule, hash::String_hash, std::equal_to<>>;

public:
    const Api::ShaderModule & registerModule(std::string key, const runtime::Device & device, 
        rhi::shaders::Stages stage, stl::span<const std::byte> code) override {
        if (const auto * shaderModule = getModule(key)) {
            return *shaderModule;
        }
        return registerModule(std::move(key), typename Api::ShaderModule{static_cast<const Api::Device &>(device), stage, code});
    }

    const Api::ShaderModule & registerModule(std::string key, Api::ShaderModule && module) {
        return _modules.try_emplace(std::move(key), std::move(module)).first->second;
    }

    const Api::ShaderModule * getModule(std::string_view key) const noexcept override {
        if (auto it = _modules.find(key); it != _modules.end()) {
            return &it->second;
        }
        return nullptr;
    }

private:
    ModulesMap _modules;
};

}