#pragma once

#include "../rhi/common/runtime.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace engine::graphics {

class Ssao {
public:
    struct Params {
        uint8_t kernelSize = 32u;
        uint8_t noiseDim = 4u;
    };

    void createConstResources(rhi::runtime::Rhi & api, rhi::runtime::Device & device, rhi::runtime::CommandList & commandList, Params params);
    void resize(rhi::runtime::Rhi & api, rhi::runtime::Device & device, const rhi::runtime::RenderPass & renderPass, 
        uint8_t framesCount, uint32_t width, uint32_t height);
    void destroy() noexcept;

    rhi::Format format() const noexcept { return rhi::Format::eR8_Unorm; }
    std::array<rhi::Format, 1u> formatsArray() const noexcept { return {format()}; }

    rhi::runtime::FrameBufferPtr & getFrameBuffer(uint8_t idx) noexcept { return _frameBuffers[idx]; }
    rhi::runtime::ImagePtr & getImage(uint8_t idx) noexcept { return _images[idx]; }
    rhi::runtime::ImagePtr & getNoiseImage() noexcept { return _noise; }
    rhi::runtime::BufferPtr & getKernelBuffer() noexcept { return _kernelBuffer; }

private:
    rhi::runtime::BufferPtr _kernelBuffer;
    rhi::runtime::ImagePtr _noise;
    std::vector<rhi::runtime::ImagePtr> _images;
    std::vector<rhi::runtime::FrameBufferPtr> _frameBuffers;
};
    
}