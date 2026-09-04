#include "color_picker.h"
#include "graphics/render/rhi/common/command_list.h"
#include "stl/array.h"

namespace engine::graphics {
    
    ColorPicker::ColorPicker(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
        uint8_t imagesCount, vec2u size, 
        rhi::ClearValue clearColorValue,
        rhi::Format format) :
        _framebuffers(imagesCount),
        _api(api),
        _device(device),
        _clearColorValue(clearColorValue),
        _format(format),
        _size(size),
        _imagesCount(imagesCount),
        _imageReader(api.createImageMemoryReader(device, format, {size.x, size.y, 1u}, imagesCount)) {
        using namespace rhi;
        const auto access = _device.isFormatSupported(_format, FormatFeatureFlags::eFormatFeature_Color_Attachment | FormatFeatureFlags::eFormatFeature_Transfer_Src);
        assert(access.checkBit(FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit));
    }

    void ColorPicker::createRenderPass(rhi::Format depthFormat, rhi::ImageAspectFlags depthAspectFlags) {
        using namespace rhi;

        // renderpass
        stl::array attachment = {
            AttachmentDescription {
                .format = _format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eTransfer_Src_Optimal,
                .samples = 1u
            },
            AttachmentDescription {
                .format = depthFormat,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eLoad, AttachmentStoreOp::eDont_Care},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                .finalLayout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                .samples = 1u
            },
        };

        stl::array subpass = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .depthStencilAttachment = {
                    .pNext = {nullptr, 0u},
                    .attachment = 1u,
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                    .aspectMask = depthAspectFlags
                },
                .colorAttachments = {{
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eColor_Attachment_Optimal,
                    .aspectMask = ImageAspectFlags::eColor
                }}
            }
        };

        // stl::array<SubpassDependency, 1u> dependency;
        // dependency[0u] = {
        //     .stages = {
        //         .src = {
        //             .stage = PipelineStageFlags::eLate_Fragment_Tests,
        //             .accessMask = AccessFlags::eDepth_Stencil_Attachment_Write
        //         },
        //         .dst = {
        //             .stage = PipelineStageFlags::eEarly_Fragment_Tests,
        //             .accessMask = AccessFlags::eDepth_Stencil_Attachment_Read
        //         }
        //     }
        // };

        const RenderPassDescription description{attachment, subpass, {}};
        _renderPass = _api.getRenderPass(_device, description);
    }

    void ColorPicker::createRenderData(stl::span<engine::rhi::runtime::ImagePtr> depthImages) {
        using namespace rhi;

        assert(_renderPass != {});

        const auto & depthImageInfo = depthImages[0u]->getInfo();
        const auto depthImageFormat = depthImageInfo.format;
        const auto depthImageAspectFlags = depthImageInfo.aspectFlags;
        const auto depthImageExtent = depthImageInfo.extent;
        
        // attachments
        auto fbColorAttachment = FrameBufferAttachment {
            .image = nullptr,
            .imageSize = {_size.x, _size.y},
            .aspect = ImageAspectFlags::eColor,
            .layer = 0u,
            .level = 0u,
            .layersCount = 1u,
            .levelsCount = 1u,
            .viewType = ImageViewType::eType_2D
        };

        auto fbDepthAttachment = FrameBufferAttachment {
            .image = nullptr,
            .imageSize = {_size.x, _size.y},
            .aspect = depthImageAspectFlags,
            .layer = 0u,
            .level = 0u,
            .layersCount = 1u,
            .levelsCount = 1u,
            .viewType = ImageViewType::eType_2D
        };

        // images
        const auto imageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eColor_Attachment | ImageUsageFlags::eTransfer_Src, 
            _format, 
            {_size.x, _size.y, 1u},
            ImageAspectFlags::eColor
        };

        for (uint8_t i = 0u; i < _imagesCount; ++i) {
            auto image = _api.createImage(_device, imageInfo);
            fbColorAttachment.image = image.get();
            fbDepthAttachment.image = depthImages[i].get();

            stl::vector fbAttachemnts = {
                fbColorAttachment,
                fbDepthAttachment
            };

            _framebuffers[i] = ColorIdsFrameBuffer{
                .image = std::move(image),
                .depthImage = nullptr,
                .frameBuffer = _api.createFrameBuffer(_device, std::move(fbAttachemnts), _renderPass)
            };
        }
    }

    void ColorPicker::createRenderData() {
        using namespace rhi;

        // renderpass
        stl::array attachment = {
            AttachmentDescription {
                .format = _format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eTransfer_Src_Optimal,
                .samples = 1u
            },
            AttachmentDescription {
                .format = Format::eD32_Sfloat,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eDont_Care},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                .samples = 1u
            },
        };

        stl::array subpass = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .depthStencilAttachment = {
                    .pNext = {nullptr, 0u},
                    .attachment = 1u,
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                    .aspectMask = ImageAspectFlags::eDepth
                },
                .colorAttachments = {{
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eColor_Attachment_Optimal,
                    .aspectMask = ImageAspectFlags::eColor
                }}
            }
        };

        const RenderPassDescription description{attachment, subpass, {}};
        _renderPass = _api.getRenderPass(_device, description);
        
        // attachments
        auto fbColorAttachment = FrameBufferAttachment {
            .image = nullptr,
            .imageSize = {_size.x, _size.y},
            .aspect = ImageAspectFlags::eColor,
            .layer = 0u,
            .level = 0u,
            .layersCount = 1u,
            .levelsCount = 1u,
            .viewType = ImageViewType::eType_2D
        };

        auto fbDepthAttachment = FrameBufferAttachment {
            .image = nullptr,
            .imageSize = {_size.x, _size.y},
            .aspect = ImageAspectFlags::eDepth,
            .layer = 0u,
            .level = 0u,
            .layersCount = 1u,
            .levelsCount = 1u,
            .viewType = ImageViewType::eType_2D
        };

        // images
        const auto imageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eColor_Attachment | ImageUsageFlags::eTransfer_Src, 
            _format, {_size.x, _size.y, 1u},
            ImageAspectFlags::eColor
        };

        const auto depthImageInfo = ImageInfo{
            ImageUsageFlags::eDepth_Stencil_Attachment, 
            Format::eD32_Sfloat, {_size.x, _size.y, 1u},
            ImageAspectFlags::eDepth
        };

        for (uint8_t i = 0u; i < _imagesCount; ++i) {
            auto image = _api.createImage(_device, imageInfo);
            auto depthImage = _api.createImage(_device, depthImageInfo);
            fbColorAttachment.image = image.get();
            fbDepthAttachment.image = depthImage.get();

            stl::vector fbAttachemnts = {
                fbColorAttachment,
                fbDepthAttachment
            };

            _framebuffers[i] = ColorIdsFrameBuffer{
                .image = std::move(image),
                .depthImage = std::move(depthImage),
                .frameBuffer = _api.createFrameBuffer(_device, std::move(fbAttachemnts), _renderPass)
            };
        }
    }

    void ColorPicker::createRenderData(stl::span<engine::rhi::runtime::ImagePtr> depthImages, vec2i offset) {
        using namespace rhi;

        _frameBufferOffset = offset;
        _externalDepthImages = {depthImages.begin(), depthImages.end()};

        const auto & externalDepthImageInfo = depthImages[0u]->getInfo();
        const auto depthImageFormat = externalDepthImageInfo.format;
        const auto depthImageAspectFlags = externalDepthImageInfo.aspectFlags;

        // renderpass
        stl::array attachment = {
            AttachmentDescription {
                .format = _format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eTransfer_Src_Optimal,
                .samples = 1u
            },
            AttachmentDescription {
                .format = depthImageFormat,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eLoad, AttachmentStoreOp::eDont_Care},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                .samples = 1u
            },
        };

        stl::array subpass = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .depthStencilAttachment = {
                    .pNext = {nullptr, 0u},
                    .attachment = 1u,
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                    .aspectMask = depthImageAspectFlags
                },
                .colorAttachments = {{
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eColor_Attachment_Optimal,
                    .aspectMask = ImageAspectFlags::eColor
                }}
            }
        };

        const RenderPassDescription description{attachment, subpass, {}};
        _renderPass = _api.getRenderPass(_device, description);
        
        // attachments
        auto fbColorAttachment = FrameBufferAttachment {
            .image = nullptr,
            .imageSize = {_size.x, _size.y},
            .aspect = ImageAspectFlags::eColor,
            .layer = 0u,
            .level = 0u,
            .layersCount = 1u,
            .levelsCount = 1u,
            .viewType = ImageViewType::eType_2D
        };

        auto fbDepthAttachment = FrameBufferAttachment {
            .image = nullptr,
            .imageSize = {_size.x, _size.y},
            .aspect = depthImageAspectFlags,
            .layer = 0u,
            .level = 0u,
            .layersCount = 1u,
            .levelsCount = 1u,
            .viewType = ImageViewType::eType_2D
        };

        // images
        const auto imageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eColor_Attachment | ImageUsageFlags::eTransfer_Src, 
            _format, {_size.x, _size.y, 1u},
            ImageAspectFlags::eColor
        };

        const auto depthImageInfo = ImageInfo{
            ImageUsageFlags::eDepth_Stencil_Attachment, 
            depthImageFormat, {_size.x, _size.y, 1u},
            depthImageAspectFlags
        };

        for (uint8_t i = 0u; i < _imagesCount; ++i) {
            auto image = _api.createImage(_device, imageInfo);
            auto depthImage = _api.createImage(_device, depthImageInfo);
            fbColorAttachment.image = image.get();
            fbDepthAttachment.image = depthImage.get();

            stl::vector fbAttachemnts = {
                fbColorAttachment,
                fbDepthAttachment
            };

            _framebuffers[i] = ColorIdsFrameBuffer{
                .image = std::move(image),
                .depthImage = std::move(depthImage),
                .frameBuffer = _api.createFrameBuffer(_device, std::move(fbAttachemnts), _renderPass)
            };
        }
    }

    void ColorPicker::beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId, vec2u currentSize) noexcept {
        using namespace rhi;

        if (_frameBufferOffset != vec2i{0, 0}) {
            commandList.copyImage2Image(
                _externalDepthImages[frameId],
                _framebuffers[frameId].depthImage,
                {
                    .srcSubresource = {
                        .aspectMask = ImageAspectFlags::eDepth,
                        .mipLevel = 0u,
                        .baseArrayLayer = 0u,
                        .layerCount = 1u
                    },
                    .dstSubresource = {
                        .aspectMask = ImageAspectFlags::eDepth,
                        .mipLevel = 0u,
                        .baseArrayLayer = 0u,
                        .layerCount = 1u
                    },
                    .srcOffset = {_frameBufferOffset.x, _frameBufferOffset.y, 0},
                    .dstOffset = {0, 0, 0},
                    .extent = {_size.x, _size.y, 1u}
                }
            );
        }

        commandList.setViewport(0.0f, 0.0f, currentSize.x, currentSize.y);
        commandList.setScissor({{0, 0}, {_size.y, _size.y}});

        const auto clearValues = stl::array{
            _clearColorValue,                   // color
            rhi::ClearValue{0.0f, 0u}           // depth
        };

        commandList.beginRenderPass(_renderPass, _framebuffers[frameId].frameBuffer, clearValues, {{0, 0}, {_size.x, _size.y}});
    }

    void ColorPicker::endRenderPass(rhi::runtime::CommandList & commandList) noexcept {
        commandList.endRenderPass();
    }

    void ColorPicker::prepareResults(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept {
        _imageReader->prepareMemory(commandList, _framebuffers[frameId].image, frameId);
    }

    const rhi::runtime::ImagePtr & ColorPicker::getImage(uint8_t frameId) const noexcept {
        return _framebuffers[frameId].image;
    }

    rhi::runtime::ImagePtr & ColorPicker::getImage(uint8_t frameId) noexcept {
        return _framebuffers[frameId].image;
    }
}