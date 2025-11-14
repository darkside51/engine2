#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/runtime.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace engine::graphics {
struct ImageHolder {
    rhi::runtime::Image * image = nullptr;
    uint32_t count = 0u;
};

struct ImageHolderEqualTo {
    using is_transparent = void;

    constexpr auto operator()(const rhi::runtime::Image * img, const graphics::ImageHolder & holder) const noexcept { 
        return img == holder.image;
    }
    constexpr auto operator()(const graphics::ImageHolder & holder0, const graphics::ImageHolder & holder1) const noexcept { 
        return holder0.image == holder1.image;
    }
};
}

namespace engine::hash {
template<>
struct Hasher<graphics::ImageHolder> {
    using hash_type = std::hash<const rhi::runtime::Image*>;
    using is_transparent = void;

    std::size_t operator()(const rhi::runtime::Image* image) const { return hash_type{}(image); }
    std::size_t operator()(const graphics::ImageHolder & image) const { return hash_type{}(image.image); }
};

using ImageHolder_hash = Hasher<graphics::ImageHolder>;
}

namespace engine::graphics {

struct Frame2D {
    std::array<float, 2u> uvMin = {0.0f, 0.0f};
    std::array<float, 2u> uvMax = {1.0f, 1.0f};
};

class ImageFrame2D {
public:
    ImageFrame2D(const rhi::runtime::ImagePtr & image, const Frame2D & frame) : _image(image), _frame(frame) {}

    const rhi::runtime::ImagePtr & getImage() const noexcept {
        return _image;
    }

    const Frame2D & getFrame() const noexcept {
        return _frame;
    }
    
private:
    const rhi::runtime::ImagePtr _image = nullptr;
    Frame2D _frame = {};
};

class TextureManager {
    using ImageFramesMap = std::unordered_map<std::string, ImageFrame2D, hash::String_hash, std::equal_to<>>;
    using Images = std::vector<rhi::runtime::ImagePtr>;
    using ImagesSet = std::unordered_set<ImageHolder, hash::ImageHolder_hash, graphics::ImageHolderEqualTo>;

public:
    ~TextureManager() { destroy(); }
    void createDefaultResources(const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device, 
        const rhi::runtime::CommandListPtr & commandList);
    void destroy();

    bool addFrame(std::string_view name, const rhi::runtime::ImagePtr & image, const Frame2D & frame = {});
    bool removeFrame(std::string_view name) noexcept;

    const rhi::runtime::ImagePtr & getImage(uint32_t idx) const noexcept;
    const ImageFrame2D * findFrame(std::string_view name) const noexcept;

    const rhi::runtime::ImagePtr & getDefaultImage() const noexcept { return _defaultImage; }
    const rhi::runtime::ImagePtr & getWhiteImage() const noexcept { return _whiteImage; }
    const rhi::runtime::ImagePtr & getBlackImage() const noexcept { return _blackImage; }
    const rhi::runtime::ImagePtr & getTransparentImage() const noexcept { return _transparentImage; }

private:
    rhi::runtime::ImagePtr _defaultImage;
    rhi::runtime::ImagePtr _whiteImage;
    rhi::runtime::ImagePtr _blackImage;
    rhi::runtime::ImagePtr _transparentImage;

    Images _images;
    ImagesSet _imagesSet;
    ImageFramesMap _frames2D;
};

}