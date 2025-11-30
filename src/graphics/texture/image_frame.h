#pragma once

#include "graphics/render/rhi/common/runtime.h"

namespace engine::graphics {

struct Frame2D {
    std::array<float, 2u> uvMin = {0.0f, 0.0f};
    std::array<float, 2u> uvMax = {1.0f, 1.0f};
};

class ImageFrame2D {
public:
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

}