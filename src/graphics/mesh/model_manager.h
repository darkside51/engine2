#pragma once

#include "common/hash.h"
#include "gltf_loader.h"
#include "mesh_data.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace engine::graphics {

class ModelManager {
    struct ModelInfo {
        const models::Mesh_Data * data = nullptr;
        uint32_t index = 0xff'ff'ff'ffu;
    };
    using ModelsMap = std::unordered_map<std::string, ModelInfo, hash::String_hash, std::equal_to<>>;
    using Models = std::vector<models::Mesh_Data>;

public:
    ~ModelManager() { destroy(); }
    void destroy();

    uint32_t addModel(std::string_view name, const models::Layout & layout);
    bool removeModel(std::string_view name);
    
    ModelInfo getModelData(std::string_view name) const noexcept {
        if (auto it = _modelsMap.find(name); it != _modelsMap.end()) {
            return it->second;
        }
        return {};
    }

    const models::Mesh_Data & getModelData(uint32_t idx) const noexcept { 
        assert(idx < _models.size());
        return _models[idx];
    }

private:
    ModelsMap _modelsMap;
    Models _models;
};

}