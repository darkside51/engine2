#pragma once

#include "allocators/pool_allocator.h"
#include "common/hash.h"
#include "gltf_loader.h"
#include "mesh_data.h"
#include "graphics/render/rhi/common/runtime.h"

#include <array>
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

namespace engine {
    class FileManager;
}

namespace engine::graphics {

class GraphicsService;

constexpr auto kDefaultSemantics = std::array {
    models::AttributeSemantic::ePosition, 
    models::AttributeSemantic::eNormal, 
    models::AttributeSemantic::eUv0,
    models::AttributeSemantic::eJoints,
    models::AttributeSemantic::eWeights
};

class ModelManager {
    using Allocator = allocators::PoolAllocator<sizeof(models::MeshData), alignof(models::MeshData), false>;
    using ModelsMap = std::unordered_map<std::string, uint32_t, hash::String_hash, std::equal_to<>>;
    using Models = std::vector<models::MeshData*>;

public:
    struct ModelBuffers {
        std::vector<rhi::runtime::BufferPtr> vertexBuffers;
        rhi::runtime::BufferPtr indexBuffer = nullptr;
    };

    ModelManager();
    ~ModelManager() { destroy(); }
    void destroy();

    uint32_t addModel(std::string_view name, const models::Layout & layout, 
        GraphicsService & graphicsService, const FileManager & fileManager,
        std::span<const graphics::models::AttributeSemantic> attributes = kDefaultSemantics);
    bool removeModel(std::string_view name);
    
    uint32_t getModelDataIdx(std::string_view name) const noexcept {
        if (auto it = _modelsMap.find(name); it != _modelsMap.end()) {
            return it->second;
        }
        return 0xff'ff'ff'ffu;
    }

    const models::MeshData & getModelData(uint32_t idx) const noexcept { 
        assert(idx < _models.size());
        return *_models[idx];
    }

    const ModelBuffers & getModelBuffers(uint32_t idx) const noexcept {
        assert(idx < _gpuData.size());
        return _gpuData[idx];
    }

    void generateGpuBuffers(rhi::runtime::Rhi & api, rhi::runtime::Device & device, rhi::runtime::CommandListPtr & commandList);

private:
    Allocator _allocator{1024u};
    ModelsMap _modelsMap;
    Models _models;
    std::vector<ModelBuffers> _gpuData;
    std::vector<std::pair<uint32_t, models::MeshBuffersData>> _tmpData;
};

}