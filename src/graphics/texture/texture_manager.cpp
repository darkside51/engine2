#include "texture_manager.h"

#include <cassert>

namespace engine::graphics {

    void TextureManager::destroy() {
        _imagesSet.clear();
        _frames2D.clear();
        _images.clear();

        _defaultImage = nullptr;
        _whiteImage = nullptr;
        _blackImage = nullptr;
        _transparentImage = nullptr;
    }

    bool TextureManager::addFrame(std::string_view name, const rhi::runtime::ImagePtr & image, const Frame2D & frame) {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
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
        return nullptr;
    }

}