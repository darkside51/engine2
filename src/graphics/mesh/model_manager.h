#pragma once

#include "allocators/pool_allocator.h"
#include "common/hash.h"
#include "gltf_loader.h"
#include "mesh_data.h"
#include "graphics/common/material.h"
#include "graphics/render/rhi/common/runtime.h"

#include "stl/array.h"
#include "stl/span.h"
#include "stl/vector.h"
#include "stl/unordered_map.h"
#include "stl/utility.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace engine {
    class FileManager;
}

namespace engine::graphics {

class MaterialManager;
class GraphicsService;

constexpr auto kDefaultSemantics = stl::array {
    models::AttributeSemantic::ePosition, 
    models::AttributeSemantic::eNormal, 
    models::AttributeSemantic::eUv0,
    models::AttributeSemantic::eJoints,
    models::AttributeSemantic::eWeights
};

constexpr auto kDefaultSemantics2 = stl::array {
    models::AttributeSemantic::ePosition, 
    models::AttributeSemantic::eNormal, 
    models::AttributeSemantic::eTangent, 
    models::AttributeSemantic::eUv0,
    models::AttributeSemantic::eJoints,
    models::AttributeSemantic::eWeights
};

class ModelManager {
    inline static constexpr uint32_t kUnknownDataIndex = 0xff'ff'ff'ffu;

public:
    struct ModelBuffers {
        stl::vector<rhi::runtime::BufferPtr> vertexBuffers;
        rhi::runtime::BufferPtr indexBuffer = nullptr;
        rhi::runtime::BufferPtr uniqueIndicesBuffer = nullptr;
    };

    ModelManager(MaterialManager & materialManager);
    ~ModelManager() { destroy(); }
    void destroy() noexcept;

    stl::pair<uint32_t, bool> addModel(std::string_view name, models::Layout && layout, 
        GraphicsService & graphicsService, const FileManager & fileManager,
        stl::span<const graphics::models::AttributeSemantic> attributes = kDefaultSemantics2);

    stl::pair<uint32_t, bool> addModel2(std::string_view name);    

    void setBuffers(uint32_t index, models::MeshBuffersData && buffers);

    models::MeshBuffersData loadModel(uint32_t index, std::string_view name, const models::Layout & layout, 
        stl::span<const graphics::models::AttributeSemantic> attributes = kDefaultSemantics2);

    stl::vector<MaterialTextureLoadingData> prepareMaterials(
        std::string_view name, uint32_t index, 
        const models::Layout & layout, 
        TextureManager & textureManager);

    bool removeModel(std::string_view name);
    
    uint32_t getModelDataIdx(std::string_view name) const noexcept {
        if (auto it = _modelsMap.find(name); it != _modelsMap.end()) {
            return it->second;
        }
        return kUnknownDataIndex;
    }

    const models::MeshData & getModelData(uint32_t idx) const noexcept { 
        assert(idx < _models.size());
        return *_models[idx].first;
    }

    const ModelBuffers & getModelBuffers(uint32_t idx) const noexcept {
        assert(idx < _gpuData.size());
        return _gpuData[idx];
    }

    void generateGpuBuffers(
        rhi::runtime::Rhi & api, 
        rhi::runtime::Device & device, 
        rhi::runtime::CommandList & commandList
    );

    void generateGpuBuffers(
        uint32_t index, 
        models::MeshBuffersData && buffers,
        rhi::runtime::Rhi & api, 
        rhi::runtime::Device & device,
        rhi::runtime::CommandList & commandList
    );

    bool isDataReady(uint32_t idx) const noexcept {
        if (idx >= _models.size()) {
            return false;
        }
        
        return _models[idx].second;
    }

    void setDataReady(uint32_t idx, bool ready) noexcept {
        if (idx < _models.size()) {
            _models[idx].second = ready;
        }
    }

private:
    using Allocator = allocators::PoolAllocator<sizeof(models::MeshData), alignof(models::MeshData), false>;
    using ModelsMap = stl::unordered_map<std::string, uint32_t, hash::String_hash, std::equal_to<>>;
    using Models = stl::vector<stl::pair<models::MeshData*, bool>>;

    MaterialManager & _materialManager;
    Allocator _allocator{1024u};
    ModelsMap _modelsMap;
    Models _models;
    stl::vector<ModelBuffers> _gpuData;
    stl::vector<stl::pair<uint32_t, models::MeshBuffersData>> _tmpData;
};

}