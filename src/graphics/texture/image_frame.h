#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "math/math.h"
#include "texture.h"

namespace engine::graphics {

struct Frame2D {
    std::array<float, 2u> uvMin = {0.0f, 0.0f};
    std::array<float, 2u> uvMax = {1.0f, 1.0f};
};

class ImageFrame2D {
public:
    ImageFrame2D() = default;
    ImageFrame2D(const rhi::runtime::ImagePtr & image, const Frame2D & frame) : _image(image), _frame(frame) {}
    // ImageFrame2D(const ImageFrame2D &) = default;
    // ImageFrame2D(ImageFrame2D &&) = default;

    // ImageFrame2D & operator=(const ImageFrame2D &) = default;
    // ImageFrame2D & operator=(ImageFrame2D &&) = default;

    const rhi::runtime::ImagePtr & getImage() const noexcept {
        return _image;
    }

    const Frame2D & getFrame() const noexcept {
        return _frame;
    }
    
private:
    rhi::runtime::ImagePtr _image = nullptr;
    Frame2D _frame = {};
};

class ImageFrame2D2 {
public:
    ImageFrame2D2() = default;
    ImageFrame2D2(ImageHandler * image, const Frame2D & frame) : _image(image), _frame(frame) {}

    const rhi::runtime::ImagePtr & getImage() const noexcept {
        return _image->image;
    }

    const ImageHandler & getImageHandler() const noexcept {
        return *_image;
    }

    const Frame2D & getFrame() const noexcept {
        return _frame;
    }

    vec2f getSize() const noexcept {
        const auto & extent = _image->image->getExtent();
        return vec2f{_frame.uvMax[0u] - _frame.uvMin[0u], _frame.uvMax[1u] - _frame.uvMin[1u]} * vec2f{extent.width, extent.height};
    }
    
private:
    ImageHandler * _image;
    Frame2D _frame = {};
};

}