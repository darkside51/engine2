#include "texture_manager.h"

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/staging_buffer.h"
#include "graphics/render/rhi/common/command_list.h"
#include "log/log.h"

#include <cassert>

namespace engine::graphics {

    TextureManager::TextureManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device) : _api(api), _device(device) {}

    void TextureManager::destroy() {
        _imagesSet.clear();
        _frames2D.clear();
        _images.clear();
        
        _defaultFrame = {};

        _defaultImage = nullptr;
        _whiteImage = nullptr;
        _blackImage = nullptr;
        _transparentImage = nullptr;

        _defaultCubeMapImage = nullptr;
    }

    void TextureManager::createDefaultResources(const rhi::runtime::CommandListPtr & commandList) {
        using namespace rhi;

        const auto imageInfo0 = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {2u, 2u, 1u}
        };

        const auto imageInfo1 = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {1u, 1u, 1u}
        };

        _defaultImage = _api.createImage(_device, imageInfo0);
        _whiteImage = _api.createImage(_device, imageInfo1);
        _blackImage = _api.createImage(_device, imageInfo1);
        _transparentImage = _api.createImage(_device, imageInfo1);


        auto & handler = _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(_defaultImage));
        _defaultFrame = {handler.get(), {}};

        _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(_whiteImage));
        _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(_blackImage));
        _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(_transparentImage));

        constexpr auto stagingSize = sizeof(uint32_t) * 4u + 
                                     sizeof(uint32_t) * 1u + 
                                     sizeof(uint32_t) * 1u +
                                     sizeof(uint32_t) * 1u;

        StagingBuffer staging{_api, _device, stagingSize};    

        std::byte * memory = staging.map();
        const auto defaultImgOffset = staging.requestBytes(sizeof(uint32_t) * 4u);
        const auto whiteImageOffset = staging.requestBytes(sizeof(uint32_t));
        const auto blackImageOffset = staging.requestBytes(sizeof(uint32_t));
        const auto transparentImageOffset = staging.requestBytes(sizeof(uint32_t));

        auto * defaultImgData = reinterpret_cast<uint32_t*>(memory + defaultImgOffset);
        auto * whiteImgData = reinterpret_cast<uint32_t*>(memory + whiteImageOffset);
        auto * blackImgData = reinterpret_cast<uint32_t*>(memory + blackImageOffset);
        auto * transparentImgData = reinterpret_cast<uint32_t*>(memory + transparentImageOffset);

        defaultImgData[0] = 0xff'aa'aa'aau; defaultImgData[1] = 0xff'55'55'55u;
        defaultImgData[2] = 0xff'55'55'55u; defaultImgData[3] = 0xff'aa'aa'aau;

        whiteImgData[0] = 0xff'ff'ff'ffu;
        blackImgData[0] = 0xff'00'00'00u;
        transparentImgData[0] = 0x00'00'00'00u;

        staging.flush();
        staging.unmap();
        
        commandList->copyBuffer2Image(staging.getBuffer(), _defaultImage, 
            {
                .bufferOffset = defaultImgOffset,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _defaultImage->getExtent()
            }
        );

        commandList->copyBuffer2Image(staging.getBuffer(), _whiteImage, 
            {
                .bufferOffset = whiteImageOffset,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _whiteImage->getExtent()
            }
        );

        commandList->copyBuffer2Image(staging.getBuffer(), _blackImage, 
            {
                .bufferOffset = blackImageOffset,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _blackImage->getExtent()
            }
        );

        commandList->copyBuffer2Image(staging.getBuffer(), _transparentImage, 
            {
                .bufferOffset = transparentImageOffset,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _transparentImage->getExtent()
            }
        );

        std::array<TextureLayerData, 6u> layers;

        layers[0u] = {{
            std::byte{0xaau}, std::byte{0xaau}, std::byte{0xaau}, std::byte{0xffu}
        }};

        layers[1u] = {{
            std::byte{0x55u}, std::byte{0x55u}, std::byte{0x55u}, std::byte{0xffu}
        }};

        layers[2u] = {{
            std::byte{0x22u}, std::byte{0x22u}, std::byte{0x22u}, std::byte{0xffu}
        }};

        layers[3u] = {{
            std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}, std::byte{0xffu}
        }};

        layers[4u] = {{
            std::byte{0x77u}, std::byte{0x77u}, std::byte{0x77u}, std::byte{0xffu}
        }};

        layers[5u] = {{
            std::byte{0x33u}, std::byte{0x33u}, std::byte{0x33u}, std::byte{0xffu}
        }};

        _defaultCubeMapImage = generateImage(Format::eR8G8B8A8_Unorm, {1u, 1u, 1u}, TextureFlags::eNone, layers, 
            rhi::ImageCreateFlags::eCube_Compatible, *commandList);
    }

    const ImageFrame2D & TextureManager::addFrame(std::string_view name, TextureCreateInfo && info, const Frame2D & frame) {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            LOG_D("TextureManager: frame with name %s alredy exists!", name.data());
            return it->second;
        }

        if (info.metaInfo.format == rhi::Format::eUndefined) {
            return _defaultFrame;
        }

        const auto imageInfo = rhi::ImageInfo {
            rhi::ImageUsageFlags::eSampled | rhi::ImageUsageFlags::eTransfer_Dst,
            info.metaInfo.format, {info.metaInfo.size.x, info.metaInfo.size.y, 1u}
        };

        const auto image = _api.createImage(_device, imageInfo);

        ImageHandler * handler = nullptr;
        if (image) {
            auto it = _imagesSet.find(image.get());
            if (it == _imagesSet.end()) {
                handler = _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(image)).get();
                _imagesSet.emplace(handler, 1u);
            } else {
                ++const_cast<ImageHolder&>(*it).count;
                handler = it->image;
            }
        }

        const auto & [imageFrame, success] = _frames2D.emplace(name, ImageFrame2D{handler, frame});
        assert(success);

        _tmpTexturesData.emplace_back(image, std::move(info));
        return imageFrame->second;
    }

    const ImageFrame2D & TextureManager::addFrame(std::string_view name, const rhi::runtime::ImagePtr & image, const Frame2D & frame) {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            LOG_D("TextureManager: frame with name %s alredy exists!", name.data());
            return it->second;
        }

        ImageHandler * handler = nullptr;
        if (image) {
            auto it = _imagesSet.find(image.get());
            if (it == _imagesSet.end()) {
                handler = _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(image)).get();
                _imagesSet.emplace(handler, 1u);
            } else {
                ++const_cast<ImageHolder&>(*it).count;
                handler = it->image;
            }
        } else {
            handler = _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(_defaultImage)).get();
            _imagesSet.emplace(handler, 1u);
        }

        const auto & [imageFrame, success] = _frames2D.emplace(name, ImageFrame2D{handler, frame});
        assert(success);
        return imageFrame->second;
    }

    void TextureManager::addFrames(std::span<std::pair<std::string, Frame2D>> frames, const rhi::runtime::ImagePtr & image) {
        if (frames.empty() || image == nullptr) {
            return;
        }

        ImageHandler * handler = nullptr;
        auto it = _imagesSet.find(image.get());
        if (it == _imagesSet.end()) {
            handler = _images.emplace_back(rhi::runtime::sh_ptr<engine::graphics::ImageHandler>::make(image)).get();
            it = _imagesSet.emplace(handler, 0u).first;
        } else {
            handler = it->image;
        }

        uint32_t newFramesCount = 0u;
        for (const auto & [name, frame] : frames) {
            if (auto it = _frames2D.find(name); it == _frames2D.end()) {
                _frames2D.emplace(name, ImageFrame2D{handler, frame});
                ++newFramesCount;
            } else {
                LOG_D("TextureManager frame with name %s alredy exists!", name.c_str());
            }
        }

        const_cast<ImageHolder&>(*it).count += newFramesCount;
    }

    bool TextureManager::removeFrame(std::string_view name) noexcept {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            const auto * image = it->second.getImage().get();

            if (auto it1 = _imagesSet.find(image); it1 != _imagesSet.end()) {
                if (--const_cast<ImageHolder&>(*it1).count == 0u) {
                    _imagesSet.erase(it1);
                    std::erase_if(_images, [image](const ImageHandlerPtr & img) noexcept {
                        return img->image.get() == image;
                    });
                }
            }

            _frames2D.erase(it);
            return true;
        }
        return false;
    }

    const rhi::runtime::ImagePtr & TextureManager::getImage(uint32_t idx) const noexcept {
        // so unsafe...
        // do not use it!
        assert(idx < _images.size());
        return _images[idx]->image;
    }

    std::pair<ImageHandler *, bool> TextureManager::addImage(std::string_view name) {
        if (auto result = getImageHandler(name)) {
            return {result, false};
        }

        const ImageFrame2D & frame = addFrame(name, nullptr, {});
        return {const_cast<ImageHandler *>(frame.getImageHandlerPtr()), true};
    }

    ImageHandlerPtr & TextureManager::getImageHandler(uint32_t idx) noexcept {
        // so unsafe...
        // do not use it!
        assert(idx < _images.size());
        return _images[idx];
    }

    ImageHandler * TextureManager::getImageHandler(std::string_view name) noexcept {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            return const_cast<ImageHandler *>(it->second.getImageHandlerPtr());
        }
        return nullptr;
    }

    const ImageFrame2D & TextureManager::findFrame(std::string_view name) const noexcept {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            return it->second;
        }
        return _defaultFrame;
    }

    void TextureManager::generateGpuData(rhi::runtime::CommandListPtr & commandList) {
        using namespace rhi;
        if (_tmpTexturesData.empty()) {
            return;
        }

        uint32_t allDataSize = 0u;
        for (const auto & [_, info] : _tmpTexturesData) {
            allDataSize += info.data[0u].size();
        }

        StagingBuffer staging{_api, _device, allDataSize};
        auto & stagingBuffer = staging.getBuffer();
        std::byte * memory = staging.map();

        for (auto & [image, info] : _tmpTexturesData) {
            auto const sizeInBytes = info.data[0u].size();
            const auto bufferOffset = staging.requestBytes(sizeInBytes);
            auto * data = (memory + bufferOffset);
            std::memcpy(data, info.data[0u].data(), sizeInBytes);

            commandList->copyBuffer2Image(staging.getBuffer(), image, 
                {
                    .bufferOffset = bufferOffset,
                    .bufferRowLength = 0u,
                    .bufferImageHeight = 0u,
                    .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                    .imageOffset = {0u, 0u, 0u},
                    .imageExtent = image->getExtent()
                }
            );
        }

        staging.flush();
        staging.unmap();

        _tmpTexturesData.clear();
    }

    rhi::runtime::ImagePtr TextureManager::generateImage(rhi::Format format, const vec3u & size, TextureFlags flags, std::span<TextureLayerData> layersData, 
        rhi::ImageCreateFlags createFlags, rhi::runtime::CommandList & commandList) const {
        using namespace rhi;

        bool useMipMaps = (flags & TextureFlags::eUseMipMaps) != TextureFlags::eNone;
        const auto formatBlitSupport = _device.isFormatSupported(format, 
            engine::rhi::FormatFeatureFlags::eFormatFeature_BLit_Dst);
        const bool blitAvailable = formatBlitSupport.features != 0u;

        uint8_t mipLevels = layersData[0u].size();
        const uint8_t layersCount = layersData.size();

        if (useMipMaps && mipLevels == 1u && blitAvailable) {
            mipLevels = static_cast<uint8_t>(
                            std::floor(
                                std::log2(std::max(size.x, size.y))
                            )
                        ) + 1u;
            useMipMaps = mipLevels > 1u;
        }

        const ImageUsageFlags usage = useMipMaps ? 
                                      ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Src | ImageUsageFlags::eTransfer_Dst : 
                                      ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst;

        const auto imageInfo = ImageInfo{
            usage,
            format, 
            {size.x, size.y, size.z},
            ImageAspectFlags::eColor, ImageType::eType_2D, mipLevels, layersCount, 1u,ImageTiling::eOptimal, createFlags
        };

        auto image = _api.createImage(_device, imageInfo);

        auto stagingSize = 0u;
        for (const auto & layer : layersData) {
            for (const auto & textureMipMapLevelData : layer) {
                stagingSize += textureMipMapLevelData.size();
            }
        }
        
        engine::graphics::StagingBuffer staging{_api, _device, stagingSize}; // use general InflightStagingBuffer ?

        auto * memory = staging.map();
        const auto readyMipLevels = layersData[0u].size();

        std::vector<std::byte*> imgData;
        std::vector<uint32_t> mipOffsets;
        imgData.resize(layersCount * readyMipLevels);
        mipOffsets.resize(layersCount * readyMipLevels);

        uint8_t j = 0u;
        for (const auto & layer : layersData) {
            for (uint8_t i = 0u; i < readyMipLevels; ++i) {
                const size_t dstIndex = i + j * readyMipLevels;
                mipOffsets[dstIndex] = staging.requestBytes(layer[i].size());
                imgData[dstIndex] = memory + mipOffsets[dstIndex];
                std::memcpy(imgData[dstIndex], layer[i].data(), layer[i].size());
            }
            ++j;
        }
                    
        staging.flush();

        j = 0u;
        for (const auto & layer : layersData) {
            auto extent = image->getExtent();
            for (uint8_t i = 0u; i < readyMipLevels; ++i) {
                const size_t srcIndex = i + j * readyMipLevels;
                commandList.copyBuffer2Image(staging.getBuffer(), image, 
                    {
                        .bufferOffset = mipOffsets[srcIndex],
                        .bufferRowLength = 0u,
                        .bufferImageHeight = 0u,
                        .imageSubresource = {
                            .aspectMask = engine::rhi::ImageAspectFlags::eColor,
                            .mipLevel = i,
                            .baseArrayLayer = j,
                            .layerCount = 1u
                        },
                        .imageOffset = {0u, 0u, 0u},
                        .imageExtent = extent
                    }
                );
                extent.width = std::max(extent.width >> 1u, 1u);
                extent.height = std::max(extent.height >> 1u, 1u);
            }
            ++j;
        }

        if (!useMipMaps || readyMipLevels > 1u) {
            return image;
        }

        if (blitAvailable) { // useMipMaps is true
            std::array<ImageBlit, 8u> regions;

            j = 0u;
            for (const auto & layer : layersData) {
                ImageBlit & region = regions[0u];

                auto mipWidth = static_cast<int32_t>(size.x);
                auto mipHeight = static_cast<int32_t>(size.y);

                for (uint8_t i = 1u; i < mipLevels; ++i) {
                    region.srcOffsets[0u] = { 0, 0, 0 };
                    region.srcOffsets[1u] = { mipWidth, mipHeight, 1};
                    region.srcSubresource = ImageSubresourceLayers{
                        .aspectMask = ImageAspectFlags::eColor,
                        .mipLevel = static_cast<uint8_t>(i - 1u),
                        .baseArrayLayer = j,
                        .layerCount = 1u,
                    };

                    region.dstOffsets[0u] = { 0, 0, 0 };
                    region.dstOffsets[1u] = { mipWidth > 1 ? mipWidth >> 1 : 1, mipHeight > 1 ? mipHeight >> 1 : 1, 1 };
                    region.dstSubresource = ImageSubresourceLayers{
                        .aspectMask = ImageAspectFlags::eColor,
                        .mipLevel = i,
                        .baseArrayLayer = j,
                        .layerCount = 1u,
                    };

                    BlitImageInfo blitInfo {
                        .srcImageLayout = ImageLayout::eTransfer_Src_Optimal,
                        .dstImageLayout = ImageLayout::eTransfer_Dst_Optimal,
                        .regions = regions,
                        .regionsCount = 1u,
                        .filter = FilterMode::eLinear
                    };

                    commandList.blitImage(image, image, blitInfo);

                    mipWidth = std::max(mipWidth >> 1, 1);
                    mipHeight = std::max(mipHeight >> 1, 1);
                }
                ++j;
            }
        }

        return image;
    }

    rhi::runtime::ImagePtr TextureManager::generateImage(TextureCreateInfo && textureData, rhi::runtime::CommandList & commandList) const {
        return generateImage(textureData.metaInfo.format, textureData.metaInfo.size, textureData.metaInfo.flags, 
            std::span{&textureData.data, 1u}, rhi::ImageCreateFlags::eNone, commandList);
    }

}