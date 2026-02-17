#pragma once

namespace engine::rhi::runtime {
    class Rhi;
    class Device;
}

namespace engine::graphics {
    class TextureManager;
    class ModelManager;
    class MaterialManager;

    class GraphicsService {
    public:
        GraphicsService(
            rhi::runtime::Rhi & rhi, 
            rhi::runtime::Device & device, 
            TextureManager & textureManager, 
            ModelManager & modelManager,
            MaterialManager & materialManager
        ) :
            _rhi(rhi), 
            _device(device), 
            _textureManager(textureManager), 
            _modelManager(modelManager),
            _materialManager(materialManager) {}

        const rhi::runtime::Rhi & getRhi() const noexcept { return _rhi; }
        rhi::runtime::Rhi & getRhi() noexcept { return _rhi; }

        const rhi::runtime::Device & getDevice() const noexcept { return _device; }
        rhi::runtime::Device & getDevice() noexcept { return _device; }

        const TextureManager & getTextureManager() const noexcept { return _textureManager; }
        TextureManager & getTextureManager() noexcept { return _textureManager; }

        const ModelManager & getModelManager() const noexcept { return _modelManager; }
        ModelManager & getModelManager() noexcept { return _modelManager; }

        const MaterialManager & getMaterialManager() const noexcept { return _materialManager; }
        MaterialManager & getMaterialManager() noexcept { return _materialManager; }

    private:
        rhi::runtime::Rhi & _rhi;
        rhi::runtime::Device & _device;
        TextureManager & _textureManager;
        ModelManager & _modelManager;
        MaterialManager & _materialManager;
    };
}