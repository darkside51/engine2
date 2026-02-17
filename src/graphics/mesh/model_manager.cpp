#include "model_manager.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/staging_buffer.h"

#include "graphics/common/graphics_service.h"
#include "graphics/common/material.h"

namespace engine::graphics {

    ModelManager::ModelManager(MaterialManager & materialManager) : _materialManager(materialManager) {
        _models.resize(1024u);
        _gpuData.resize(1024u);
        _tmpData.reserve(1024u);
    }

    void ModelManager::destroy() {
        for (auto * data : _models) {
            _allocator.destroy(data);
        }

        _modelsMap.clear();
        _models.clear();
        _gpuData.clear();
        _tmpData.clear();
    }

    uint32_t ModelManager::addModel(
        std::string_view name, 
        const models::Layout & layout, 
        GraphicsService & graphicsService, 
        const FileManager & fileManager, 
        std::span<const graphics::models::AttributeSemantic> attributes
    ) {
        if (const auto idx = getModelDataIdx(name); idx != 0xff'ff'ff'ffu) {
            return idx;
        }

        auto * meshData = _allocator.create<models::MeshData>();
        const auto result = _allocator.getObjectNumber(meshData);

        _modelsMap.emplace(name, result);
        if (result >= _models.size()) {
            _models.resize(result * 2u);
            _gpuData.resize(result * 2u);
        }
        _models[result] = meshData;
        _gpuData[result] = {};

        meshData->loadSkins(layout);
        meshData->loadAnimations(layout);
        //meshData->loadMaterials(layout, fileManager, graphicsService.getTextureManager(), name);

        auto buffers = meshData->loadMeshes(layout, attributes);
        meshData->optimizeBuffers(buffers.vertexBuffersData, buffers.indexBufferData, attributes);
        meshData->loadNodes(layout);
        _tmpData.emplace_back(result, std::move(buffers));

        auto remap = _materialManager.loadMaterials(name, layout, fileManager, graphicsService.getTextureManager());
        for (auto & mesh : meshData->meshes) {
            for (auto & p : mesh.primitives) {
                p.material = remap[p.material];
            }
        }

        return result;
    }

    bool ModelManager::removeModel(std::string_view name) { // need to remove materials and textures!
        if (auto it = _modelsMap.find(name); it != _modelsMap.end()) {
            const auto idx = it->second;

            for (const auto & mesh : _models[idx]->meshes) {
                for (auto const & p : mesh.primitives) {
                    _materialManager.removeMaterial(p.material);
                }
            }

            _allocator.destroy(_models[idx]);
            _gpuData[idx] = {};
            _models[idx] = nullptr;
            std::erase_if(_tmpData, [idx](const auto & tmp) noexcept { return tmp.first == idx; });

            _modelsMap.erase(it);
        }
    }

    void ModelManager::generateGpuBuffers(rhi::runtime::Rhi & api, rhi::runtime::Device & device, rhi::runtime::CommandListPtr & commandList) {
        using namespace rhi;
        using namespace graphics;
        
        if (_tmpData.empty()) {
            return;
        }

        BufferInfo verticesBufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        BufferInfo indicesBufferInfo {
            BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        uint32_t allDataSize = 0u;

        for (const auto & [index, data] : _tmpData) {
            const auto & [vertices, indices] = data;
            auto & gpuData = _gpuData[index];
            gpuData.indexBuffer = api.createBuffer(device, indicesBufferInfo, sizeof(uint32_t) * indices.size());
            allDataSize += sizeof(uint32_t) * indices.size();
            gpuData.vertexBuffers.reserve(vertices.size());

            for (const auto & vb : vertices) {
                if (const auto size = vb.size()) {
                    gpuData.vertexBuffers.emplace_back(api.createBuffer(device, verticesBufferInfo, size));
                    allDataSize += size;
                }
            }
        }

        StagingBuffer staging{api, device, allDataSize};
        auto & stagingBuffer = staging.getBuffer();
        std::byte * memory = staging.map();

        for (const auto & [index, data] : _tmpData) {
            const auto & [vertices, indices] = data;
            auto & gpuData = _gpuData[index];

            auto const sizeInBytes = gpuData.indexBuffer->sizeInBytes();
            const auto indexBufferOffset = staging.requestBytes(sizeInBytes);
            auto * idxData = (memory + indexBufferOffset);
            std::memcpy(idxData, indices.data(), sizeInBytes);

            commandList->copyBuffer2Buffer(stagingBuffer, gpuData.indexBuffer, 
                {.srcOffset = indexBufferOffset, .dstOffset = 0u, .size = sizeInBytes});

            size_t i = 0u;
            for (auto & vb : gpuData.vertexBuffers) {
                if (auto const sizeInBytes = vb->sizeInBytes(); sizeInBytes > 0u) {
                    const auto bufferOffset = staging.requestBytes(sizeInBytes);
                    auto * data = (memory + bufferOffset);
                    std::memcpy(data, vertices[i].data(), sizeInBytes);

                    commandList->copyBuffer2Buffer(stagingBuffer, vb, 
                        {.srcOffset = bufferOffset, .dstOffset = 0u, .size = sizeInBytes});
                }
                ++i;
            }
        }

        staging.flush();
        staging.unmap();

        _tmpData.clear();
    }
}