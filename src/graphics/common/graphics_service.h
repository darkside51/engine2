#pragma once

namespace engine::rhi::runtime {
    class Rhi;
    class Device;
}

namespace engine::graphics {
    // class ExecutionContextHandler;
    class TextureManager;
    class ModelManager;
    class MaterialManager;
    class PipelinesManager;
    class RenderPassManager;

    class GraphicsService {
    public:
        GraphicsService(
            rhi::runtime::Rhi & rhi, 
            rhi::runtime::Device & device,
            RenderPassManager & renderPassManager,
            PipelinesManager & pipelinesManager, 
            TextureManager & textureManager,
            MaterialManager & materialManager,
            ModelManager & modelManager
        ) : _rhi(rhi), 
            _device(device),
            _renderPassManager(renderPassManager),
            _pipelinesManager(pipelinesManager),
            _textureManager(textureManager),
            _materialManager(materialManager),
            _modelManager(modelManager) {}

        const rhi::runtime::Rhi & getRhi() const noexcept { return _rhi; }
        rhi::runtime::Rhi & getRhi() noexcept { return _rhi; }

        const rhi::runtime::Device & getDevice() const noexcept { return _device; }
        rhi::runtime::Device & getDevice() noexcept { return _device; }

        const RenderPassManager & getRenderPassManager() const noexcept { return _renderPassManager; }
        RenderPassManager & getRenderPassManager() noexcept { return _renderPassManager; }

        const PipelinesManager & getPipelinesManager() const noexcept { return _pipelinesManager; }
        PipelinesManager & getPipelinesManager() noexcept { return _pipelinesManager; }

        const TextureManager & getTextureManager() const noexcept { return _textureManager; }
        TextureManager & getTextureManager() noexcept { return _textureManager; }

        const ModelManager & getModelManager() const noexcept { return _modelManager; }
        ModelManager & getModelManager() noexcept { return _modelManager; }

        const MaterialManager & getMaterialManager() const noexcept { return _materialManager; }
        MaterialManager & getMaterialManager() noexcept { return _materialManager; }

        void destroy() noexcept;

    private:
        rhi::runtime::Rhi & _rhi;
        rhi::runtime::Device & _device;
        RenderPassManager & _renderPassManager;
        PipelinesManager & _pipelinesManager;
        TextureManager & _textureManager;
        MaterialManager & _materialManager;
        ModelManager & _modelManager;
    };
}