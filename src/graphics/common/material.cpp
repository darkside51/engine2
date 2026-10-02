#include "material.h"

#include "file/file_manager.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/staging_buffer.h"
#include "graphics/texture/texture_manager.h"

#include <cassert>

namespace engine::graphics {

    void MaterialManager::destroy() noexcept {
        for (auto * data : _materials) {
            if (data) {
                _allocator.destroy(data);
                data = nullptr;
            }
        }
    }

    void MaterialManager::removeMaterial(uint32_t idx) noexcept {
        if (_materials[idx]) {
            _allocator.destroy(_materials[idx]);
            _materials[idx] = nullptr;
        }
    }

    stl::vector<uint32_t> MaterialManager::prepareMaterials(
        std::string_view name, 
        const models::Layout & layout, 
        TextureManager & textureManager,
        stl::vector<MaterialTextureLoadingData> & texturesToLoad
    ) { // render thread
        stl::vector<uint32_t> materialsRemap;
        materialsRemap.reserve(layout.materials.size());

        texturesToLoad.reserve(layout.materials.size() * kCommonMaterialTexturesCount);

        const TextureLoadInfo loadingInfoColor{
            .flags = TextureFlags::eAllowSrgbColorSpace | TextureFlags::eUseMipMaps
        };
        const TextureLoadInfo loadingInfoCommon{
            .flags = TextureFlags::eUseMipMaps
        };

        uint16_t materialIndex = 0u;
        for (const auto & srcMaterial : layout.materials) {
            auto & material = *_allocator.create<Material>();
            const auto materialId = _allocator.getObjectNumber(&material);
            _materials[materialId] = &material;

            materialsRemap.emplace_back(materialId);

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
                material.blendMode = rhi::common_blend_modes::blend_none;
                // use alphaCutoff to discard fragments
                break;
            case models::AlphaMode::eBlend:
                material.blendMode = rhi::common_blend_modes::blend_alpha_pma; // ?
                break;
            }

            material.cullMode = srcMaterial.doubleSided ? rhi::CullMode::eNone : rhi::CullMode::eBack;

            const auto getTexture = [&layout, &textureManager, &texturesToLoad, materialIndex](TextureLoadInfo loadingInfo, uint16_t index, std::string_view name, ImageHandler* defaultHandler) -> ImageHandler* {
                if (index == models::kInvalidValue) {
                    return defaultHandler;
                }

                const auto & texture = layout.textures[index];
                const auto & image = layout.images[texture.source];

                if (!image.name.empty()) {
                    const auto & [imageHandler, isNew] = textureManager.addImage(image.name);
                    if (isNew) {
                        texturesToLoad.emplace_back(nullptr, imageHandler, materialIndex, index, loadingInfo);
                    }
                    return imageHandler;
                } else {
                    const auto & [imageHandler, isNew] = textureManager.addImage(std::string{name} + std::to_string(index));
                    if (isNew) {
                        texturesToLoad.emplace_back(nullptr, imageHandler, materialIndex, index, loadingInfo);
                    }
                    return imageHandler;
                }

                return textureManager.getDefaultImageHandler();
            };

            material.textures[static_cast<uint8_t>(PbrTextureId::eBaseColor)] = getTexture(loadingInfoColor, srcMaterial.pbrMetallicRoughness.baseColorTexture.index, name, textureManager.getWhiteImageHandler());
            material.textures[static_cast<uint8_t>(PbrTextureId::eEmissive)] = getTexture(loadingInfoColor, srcMaterial.emissiveTexture.index, name, textureManager.getWhiteImageHandler());
            material.textures[static_cast<uint8_t>(PbrTextureId::eNormal)] = getTexture(loadingInfoCommon, srcMaterial.normalTexture.index, name, textureManager.getWhiteImageHandler());
            material.textures[static_cast<uint8_t>(PbrTextureId::eMetallicRoughness)] = getTexture(loadingInfoCommon, srcMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, name, textureManager.getWhiteImageHandler());
            material.textures[static_cast<uint8_t>(PbrTextureId::eOccolusion)] = getTexture(loadingInfoCommon, srcMaterial.occlusionTexture.index, name, textureManager.getTransparentImageHandler());

            ++materialIndex;
        }

        return materialsRemap;
    }

    void MaterialManager::createMaterialTextures(
        const models::Layout & layout, 
        stl::vector<MaterialTextureLoadingData> & texturesToLoad, 
        const FileManager & fileManager,
        const TextureManager & textureManager,
        rhi::runtime::CommandList & commandList
    ) {
        for (auto & loadingData : texturesToLoad) {
            const auto & srcMaterial = layout.materials[loadingData.materialId];
            const auto & texture = layout.textures[loadingData.textureIndex];
            const auto & image = layout.images[texture.source];

            TextureCreateInfo textureData;
            if (image.bufferView != models::kInvalidValue) {
                const auto & bufferView = layout.bufferViews[image.bufferView];
                const auto & buffer = layout.buffers[bufferView.buffer];
                const std::byte * begin = &buffer.data[bufferView.offset];
                textureData = parseTextureData({begin, begin + bufferView.length}, loadingData.loadingInfo);
            } else if (!image.uri.empty()) {
                textureData = parseTextureData(fileManager.readFile(image.uri), loadingData.loadingInfo);
            }

            const auto formatSupport = textureManager.getDevice().isFormatSupported(
                        textureData.metaInfo.format, 
                        rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image
                    );
            const bool supportLinearTiling = (loadingData.loadingInfo.flags & TextureFlags::eSupportLinearTiling) != TextureFlags::eNone;

            const bool sampledSupport = supportLinearTiling ?
                                        (
                                        formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit)
                                        ||
                                        formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eLinearTilingFeaturesBit)
                                        ):
                                        formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit);

            if (!sampledSupport) {
                const bool allowSrgb = (loadingData.loadingInfo.flags & TextureFlags::eAllowSrgbColorSpace) != TextureFlags::eNone;
                convertTextureDataFormat(allowSrgb ? rhi::Format::eR8G8B8A8_Srgb : rhi::Format::eR8G8B8A8_Unorm, textureData); // ??
            }

            loadingData.image = textureManager.generateImage(std::move(textureData), commandList);
        }

    }

    stl::vector<uint32_t> MaterialManager::loadMaterials(
        std::string_view name, 
        const models::Layout & layout, 
        const FileManager & fileManager, 
        TextureManager & textureManager
    ) {
        stl::vector<uint32_t> materialsRemap;
        materialsRemap.reserve(layout.materials.size());

        const TextureLoadInfo loadingInfoColor{
            .flags = TextureFlags::eAllowSrgbColorSpace | TextureFlags::eUseMipMaps
        };
        const TextureLoadInfo loadingInfoCommon{
            .flags = TextureFlags::eUseMipMaps
        };

        for (const auto & srcMaterial : layout.materials) {
            // render thread
            auto & material = *_allocator.create<Material>();
            const auto materialId = _allocator.getObjectNumber(&material);
            _materials[materialId] = &material;
            //

            materialsRemap.emplace_back(materialId);

            const auto getTexture = [&layout, &fileManager, &textureManager](const TextureLoadInfo & loadingInfo, uint16_t index, std::string_view name) -> const ImageHandler* {

                const auto createTexture = [&textureManager, loadingInfo](
                    std::string_view name, const rhi::runtime::Device & device, stl::span<const std::byte> bytes, const TextureLoadInfo & info = {}
                ) {
                    auto textureData = parseTextureData(bytes, info);

                    const auto formatSupport = device.isFormatSupported(
                        textureData.metaInfo.format, 
                        rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image
                    );
                    const bool supportLinearTiling = (info.flags & TextureFlags::eSupportLinearTiling) != TextureFlags::eNone;

                    const bool sampledSupport = supportLinearTiling ?
                                                (
                                                formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit)
                                                ||
                                                formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eLinearTilingFeaturesBit)
                                                ):
                                                formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit);

                    if (!sampledSupport) {
                        const bool allowSrgb = (loadingInfo.flags & TextureFlags::eAllowSrgbColorSpace) != TextureFlags::eNone;
                        convertTextureDataFormat(allowSrgb ? rhi::Format::eR8G8B8A8_Srgb : rhi::Format::eR8G8B8A8_Unorm, textureData); // ??
                    }

                    return textureManager.addFrame(name, std::move(textureData)).getImageHandlerPtr(); // must be on render thread only!
                };

                if (index == models::kInvalidValue) {
                    return textureManager.getDefaultImageHandler();
                }

                const auto & texture = layout.textures[index];
                const auto & image = layout.images[texture.source];

                if (image.bufferView != models::kInvalidValue) {
                    const auto & bufferView = layout.bufferViews[image.bufferView];
                    const auto & buffer = layout.buffers[bufferView.buffer];
                    const std::byte * begin = &buffer.data[bufferView.offset];

                    if (!image.name.empty()) {
                        return createTexture(image.name, textureManager.getDevice(), {begin, begin + bufferView.length}, loadingInfo);
                    } else {
                        return createTexture(std::string{name} + std::to_string(index), textureManager.getDevice(), {begin, begin + bufferView.length}, loadingInfo);
                    }
                }
                if (!image.uri.empty()) {
                    const auto imageBytes = fileManager.readFile(image.uri);

                    if (!image.name.empty()) {
                        return createTexture(image.name, textureManager.getDevice(), imageBytes, loadingInfo);
                    } else {
                        return createTexture(std::string{name} + std::to_string(index), textureManager.getDevice(), imageBytes, loadingInfo);
                    }
                }

                assert(false);
                return textureManager.getDefaultImageHandler();
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
                material.blendMode = rhi::common_blend_modes::blend_none;
                // use alphaCutoff to discard fragments
                break;
            case models::AlphaMode::eBlend:
                material.blendMode = rhi::common_blend_modes::blend_alpha_pma; // ?
                break;
            }

            material.cullMode = srcMaterial.doubleSided ? rhi::CullMode::eNone : rhi::CullMode::eBack;

            material.textures[static_cast<uint8_t>(PbrTextureId::eBaseColor)] = getTexture(loadingInfoColor, srcMaterial.pbrMetallicRoughness.baseColorTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eEmissive)] = getTexture(loadingInfoColor, srcMaterial.emissiveTexture.index, name);         
            material.textures[static_cast<uint8_t>(PbrTextureId::eNormal)] = getTexture(loadingInfoCommon, srcMaterial.normalTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eMetallicRoughness)] = getTexture(loadingInfoCommon, srcMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, name);
            material.textures[static_cast<uint8_t>(PbrTextureId::eOccolusion)] = getTexture(loadingInfoCommon, srcMaterial.occlusionTexture.index, name);
            
            // material.textures[static_cast<uint8_t>(PbrTextureId::eBaseColor)] = textureManager.getDefaultImage();
            // material.textures[static_cast<uint8_t>(PbrTextureId::eNormal)] = textureManager.getDefaultImage();
            // material.textures[static_cast<uint8_t>(PbrTextureId::eMetallicRoughness)] = textureManager.getDefaultImage();
            // material.textures[static_cast<uint8_t>(PbrTextureId::eOccolusion)] = textureManager.getDefaultImage();
            // material.textures[static_cast<uint8_t>(PbrTextureId::eEmissive)] = textureManager.getDefaultImage();
        }

        return materialsRemap;
    }

}