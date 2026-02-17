#include "material.h"

#include "file/file_manager.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/staging_buffer.h"
#include "graphics/texture/texture_manager.h"

namespace engine::graphics {

    void MaterialManager::destroy() {
        for (auto * data : _materials) {
            _allocator.destroy(data);
        }
    }

    void MaterialManager::removeMaterial(uint32_t idx) noexcept {
        _allocator.destroy(_materials[idx]);
        _materials[idx] = nullptr;
    }

    std::vector<uint32_t> MaterialManager::loadMaterials(
        std::string_view name, 
        const models::Layout & layout, 
        const FileManager & fileManager, 
        TextureManager & textureManager
    ) {
        std::vector<uint32_t> materialsRemap;
        materialsRemap.reserve(layout.materials.size());

        for (const auto & srcMaterial : layout.materials) {
            auto & material = *_allocator.create<Material>();
            const auto materialId = _allocator.getObjectNumber(&material);
            _materials[materialId] = &material;
            materialsRemap.emplace_back(materialId);

            auto const getTexture = [&layout, &fileManager, &textureManager](uint16_t index, std::string_view name) -> rhi::runtime::ImagePtr {
                if (index == models::kInvalidValue) {
                    return nullptr;
                }

                const auto & texture = layout.textures[index];
                const auto & image = layout.images[texture.source];

                if (image.bufferView != models::kInvalidValue) {
                    const auto & bufferView = layout.bufferViews[image.bufferView];
                    const auto & buffer = layout.buffers[bufferView.buffer];
                    const std::byte * begin = &buffer.data[bufferView.offset];
                    auto textureData = engine::graphics::parseTextureData({begin, begin + bufferView.length});
                    engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Unorm, textureData); // ??
                    if (!image.name.empty()) {
                        return textureManager.addFrame(image.name, std::move(textureData)).getImage();
                    } else {
                        return textureManager.addFrame(std::string{name} + std::to_string(index), std::move(textureData)).getImage();
                    }
                }
                if (!image.uri.empty()) {
                    auto const imageBytes = fileManager.readFile(image.uri);
                    auto textureData = engine::graphics::parseTextureData(imageBytes);
                    engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Unorm, textureData); // ??
                    if (!image.name.empty()) {
                        return textureManager.addFrame(image.name, std::move(textureData)).getImage();
                    } else {
                        return textureManager.addFrame(std::string{name} + std::to_string(index), std::move(textureData)).getImage();
                    }
                }

                return nullptr;
            };

            material.baseColorFactor = {
                srcMaterial.pbrMetallicRoughness.baseColorFactor.x,
                srcMaterial.pbrMetallicRoughness.baseColorFactor.y,
                srcMaterial.pbrMetallicRoughness.baseColorFactor.z,
                srcMaterial.pbrMetallicRoughness.baseColorFactor.w
            };
            material.emissiveFactor = {
                srcMaterial.emissiveFactor.x,
                srcMaterial.emissiveFactor.y,
                srcMaterial.emissiveFactor.z,
            };
            material.metallicFactor = srcMaterial.pbrMetallicRoughness.metallicFactor;
            material.roughnessFactor = srcMaterial.pbrMetallicRoughness.roughnessFactor;
            material.alphaCutoff = srcMaterial.alphaCutoff;
            switch (srcMaterial.alphaMode) {
            case models::AlphaMode::eOpaque:
                material.blendMode = rhi::common_blend_modes::blend_none;
                break;
            case models::AlphaMode::eMask:
                material.blendMode = rhi::common_blend_modes::blend_multiply; // ?
                break;
            case models::AlphaMode::eBlend:
                material.blendMode = rhi::common_blend_modes::blend_alpha_pma; // ?
                break;
            }

            material.cullMode = srcMaterial.doubleSided ? rhi::CullMode::eNone : rhi::CullMode::eBack;

            material.textures[static_cast<uint8_t>(PbrTextureId::eBaseColor)] = getTexture(srcMaterial.pbrMetallicRoughness.baseColorTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eNormal)] = getTexture(srcMaterial.normalTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eMetallicRoughness)] = getTexture(srcMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eOccolusion)] = getTexture(srcMaterial.occlusionTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eEmissive)] = getTexture(srcMaterial.emissiveTexture.index, name);
        }

        return materialsRemap;
    }

}