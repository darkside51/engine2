#include "ssao.h"

#include "common/random.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/staging_buffer.h"
#include "math/math.h"

namespace engine::graphics {

    void Ssao::createRenderPass(rhi::runtime::Rhi & api, rhi::runtime::Device & device) {
        using namespace rhi;
        // renderpass
        std::array attachment = {
            rhi::AttachmentDescription {
                .format = format(),
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eColor_Attachment_Optimal, // ImageLayout::eUndefined
                .finalLayout = ImageLayout::eShader_Read_Only_Optimal,
                .samples = 1u
            }
        };

        std::array subpass  = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .colorAttachments = {{
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eColor_Attachment_Optimal,
                    .aspectMask = ImageAspectFlags::eColor
                }}
            }
        };

        const RenderPassDescription description{attachment, subpass, {}};
        _renderPass = api.getRenderPass(device, description);
    }

    void Ssao::createConstResources(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
        rhi::runtime::CommandList & commandList, Params params) {
        // kernel
        constexpr auto lerp = [](float a, float b, float f) noexcept {
            return a + f * (b - a);
        };

        stl::vector<vec4f> ssaoKernel(params.kernelSize);
		for (uint8_t i = 0u; i < params.kernelSize; ++i) {
			vec3f sample{common::random(-1.0f, 1.0f), common::random(-1.0f, 1.0f), common::random(0.0f, 1.0f)};
			sample = glm::normalize(sample);
			sample *= common::random(0.0f, 1.0f);
			float scale = float(i) / float(params.kernelSize);
			scale = lerp(0.1f, 1.0f, scale * scale);
			ssaoKernel[i] = {sample * scale, 0.0f};
		}

        const rhi::BufferInfo kernelBufferInfo {
            rhi::BufferUsageFlags::eUniform_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        _kernelBuffer = api.createBuffer(device, kernelBufferInfo, ssaoKernel.size() * sizeof(vec4f));

        // noise
        stl::vector<vec2f> noiseValues(params.noiseDim * params.noiseDim);
		for (size_t i = 0u; i < noiseValues.size(); ++i) {
			noiseValues[i] = {engine::common::random(-1.0f, 1.0f), engine::common::random(-1.0f, 1.0f)};
            // noiseValues[i] = {
            //     static_cast<float>(common::random(-32'767, 32'767)) / 32'767.0f, 
            //     static_cast<float>(common::random(-32'767, 32'767)) / 32'767.0f
            // };
		}

        const rhi::ImageInfo noiseImageInfo {
            rhi::ImageUsageFlags::eSampled | rhi::ImageUsageFlags::eTransfer_Dst,
            rhi::Format::eR32G32_Sfloat, {params.noiseDim, params.noiseDim, 1u}
        };

        _noise = api.createImage(device, noiseImageInfo);

        { // fill gpu data
            const auto kernelDataSize = params.kernelSize * sizeof(decltype(ssaoKernel)::value_type);
            const auto noiseDataSize = noiseValues.size() * sizeof(decltype(noiseValues)::value_type);
            const auto stagingSize = kernelDataSize + noiseDataSize;

            StagingBuffer staging{api, device, stagingSize};

            std::byte * memory = staging.map();
            const auto kernelOffset = staging.requestBytes(kernelDataSize);
            const auto noiseOffset = staging.requestBytes(noiseDataSize);

            auto * kernelData = (memory + kernelOffset);
            auto * noiseData = (memory + noiseOffset);

            std::memcpy(kernelData, ssaoKernel.data(), kernelDataSize);
            std::memcpy(noiseData, noiseValues.data(), noiseDataSize);
            staging.flush();

            commandList.copyBuffer2Buffer(staging.getBuffer(), _kernelBuffer, 
                {.srcOffset = kernelOffset, .dstOffset = 0u, .size = kernelDataSize}
            );

            commandList.copyBuffer2Image(staging.getBuffer(), _noise, 
                {
                    .bufferOffset = noiseOffset,
                    .bufferRowLength = 0u,
                    .bufferImageHeight = 0u,
                    .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                    .imageOffset = {0u, 0u, 0u},
                    .imageExtent = _noise->getExtent()
                }
            );
        }
    }

    void Ssao::resize(rhi::runtime::Rhi & api, rhi::runtime::Device & device, uint8_t framesCount, uint32_t width, uint32_t height) {
        _images.clear();
        _images.reserve(framesCount);
        _frameBuffers.clear();
        _frameBuffers.reserve(framesCount);

        auto ssaoImageInfo = rhi::ImageInfo{
            rhi::ImageUsageFlags::eColor_Attachment | rhi::ImageUsageFlags::eSampled,
            rhi::Format::eR8_Unorm, {width, height, 1u}
        };
        ssaoImageInfo.memoryExtFlags = rhi::MemoryExtendedFlags::eDedicated;

        for (uint32_t i = 0u; i < framesCount; ++i) {
            auto & image = _images.emplace_back(api.createImage(device, ssaoImageInfo));

            stl::vector<rhi::FrameBufferAttachment> fbAttachemnts{1u};
            fbAttachemnts[0u].image = image.get();
            fbAttachemnts[0u].imageSize = {image->getExtent().width, image->getExtent().height};
            _frameBuffers.emplace_back(api.createFrameBuffer(device, std::move(fbAttachemnts), _renderPass));
        }
    }

    void Ssao::beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept{
        constexpr auto clearValues = std::array{
            rhi::ClearValue{0x00'00'00'ffu},
        };
        const auto & extent = _images[frameId]->getExtent();
        commandList.beginRenderPass(_renderPass, _frameBuffers[frameId], clearValues, {{0, 0}, {extent.width, extent.height}});
    }

    void Ssao::endRenderPass(rhi::runtime::CommandList & commandList) const noexcept {
        commandList.endRenderPass();
    }

    void Ssao::destroy() noexcept {
        _kernelBuffer = {};
        _noise = {};
        _frameBuffers.clear();
        _images.clear();
    }
}