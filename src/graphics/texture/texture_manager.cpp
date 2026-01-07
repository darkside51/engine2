#include "texture_manager.h"

#include "graphics/render/staging_buffer.h"
#include "graphics/render/rhi/common/command_list.h"
#include "log/log.h"

#include <cassert>

namespace engine::graphics {

    void TextureManager::destroy() {
        _imagesSet.clear();
        _frames2D.clear();
        _images.clear();
        
        _defaultFrame = {};

        _defaultImage = nullptr;
        _whiteImage = nullptr;
        _blackImage = nullptr;
        _transparentImage = nullptr;
    }

    void TextureManager::createDefaultResources(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
        const rhi::runtime::CommandListPtr & commandList) {
        using namespace rhi;

        const auto imageInfo0 = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {2u, 2u, 1u}
        };

        const auto imageInfo1 = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {1u, 1u, 1u}
        };

        _defaultImage = api.createImage(device, imageInfo0);
        _whiteImage = api.createImage(device, imageInfo1);
        _blackImage = api.createImage(device, imageInfo1);
        _transparentImage = api.createImage(device, imageInfo1);

        _defaultFrame = {_defaultImage, {}};

        constexpr auto stagingSize = sizeof(uint32_t) * 4u + 
                                     sizeof(uint32_t) * 1u + 
                                     sizeof(uint32_t) * 1u +
                                     sizeof(uint32_t) * 1u;

        StagingBuffer staging{api, device, stagingSize};    

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
    }

    bool TextureManager::addFrame(std::string_view name, const rhi::runtime::ImagePtr & image, const Frame2D & frame) {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            LOG_D("TextureManager: frame with name %s alredy exists!", name.data());
            return false;
        }

        if (image) {
            auto it = _imagesSet.find(image.get());
            if (it == _imagesSet.end()) {
                _images.emplace_back(image);
                _imagesSet.emplace(image.get(), 1u);
            } else {
                ++const_cast<ImageHolder&>(*it).count;
            }
        }

        _frames2D.emplace(name, ImageFrame2D{image, frame});

        return true;
    }

    void TextureManager::addFrames(std::span<std::pair<std::string, Frame2D>> frames, const rhi::runtime::ImagePtr & image) {
        if (frames.empty() || image == nullptr) {
            return;
        }

        uint32_t newFramesCount = 0u;
        for (const auto & [name, frame] : frames) {
            if (auto it = _frames2D.find(name); it == _frames2D.end()) {
                _frames2D.emplace(name, ImageFrame2D{image, frame});
                ++newFramesCount;
            } else {
                LOG_D("TextureManager frame with name %s alredy exists!", name.c_str());
            }
        }

        auto it = _imagesSet.find(image.get());
        if (it == _imagesSet.end()) {
            _images.emplace_back(image);
            _imagesSet.emplace(image.get(), newFramesCount);
        } else {
            const_cast<ImageHolder&>(*it).count += newFramesCount;
        }
    }

    bool TextureManager::removeFrame(std::string_view name) noexcept {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            const auto * image = it->second.getImage().get();

            if (auto it1 = _imagesSet.find(image); it1 != _imagesSet.end()) {
                if (--const_cast<ImageHolder&>(*it1).count == 0u) {
                    _imagesSet.erase(it1);
                    std::erase_if(_images, [image](const rhi::runtime::ImagePtr & img) {
                        return img.get() == image;
                    });
                }
            }

            _frames2D.erase(it);
            return true;
        }
        return false;
    }

    const rhi::runtime::ImagePtr & TextureManager::getImage(uint32_t idx) const noexcept {
        assert(idx < _images.size());
        return _images[idx];
    }

    const ImageFrame2D * TextureManager::findFrame(std::string_view name) const noexcept {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            return &it->second;
        }
        return &_defaultFrame;
    }

}