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

        const rhi::runtime::ImagePtr * imagePtr = nullptr;

        if (image) {
            auto it = _imagesSet.find(image.get());
            if (it == _imagesSet.end()) {
                auto & imageHolder = _images.emplace_back(image, 1u);
                imagePtr = &imageHolder.image;
                _imagesSet.emplace(&imageHolder);
            } else {
                imagePtr = &(*it)->image;
                ++((*it)->count);
            }
        }

        _frames2D.emplace(name, ImageFrame2D{imagePtr, frame});

        return true;
    }

    bool TextureManager::removeFrame(std::string_view name) noexcept {
        if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            const auto & image = it->second.getImage();

            if (auto it1 = _imagesSet.find(image.get()); it1 != _imagesSet.end()) {
                if (--((*it1)->count) == 0u) {
                    _imagesSet.erase(it1);
                    std::erase_if(_images, [imagePtr = image.get()](const ImageHolder & holder) {
                        return holder.image.get() == imagePtr;
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
        return _images[idx].image;
    }

    const ImageFrame2D * TextureManager::findFrame(std::string_view name) const noexcept {
         if (auto it = _frames2D.find(name); it != _frames2D.end()) {
            return &it->second;
        }
        return nullptr;
    }

}