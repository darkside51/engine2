#pragma once

namespace engine::rhi::runtime {
    class Rhi;
    class Device;
}

namespace engine::graphics {
    class TextureManager;

    class GraphicsService {
    public:
        GraphicsService(rhi::runtime::Rhi & rhi, rhi::runtime::Device & device, TextureManager & textureManager) :
            _rhi(rhi), _device(device), _textureManager(textureManager) {}

        const rhi::runtime::Rhi & getRhi() const noexcept { return _rhi; }
        rhi::runtime::Rhi & getRhi() noexcept { return _rhi; }

        const rhi::runtime::Device & getDevice() const noexcept { return _device; }
        rhi::runtime::Device & getDevice() noexcept { return _device; }

        const TextureManager & getTextureManage() const noexcept { return _textureManager; }
        TextureManager & getTextureManage() noexcept { return _textureManager; }

    private:
        rhi::runtime::Rhi & _rhi;
        rhi::runtime::Device & _device;
        TextureManager & _textureManager;
    };
}