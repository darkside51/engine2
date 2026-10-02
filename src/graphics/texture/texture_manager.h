#pragma once

#include "common/hash.h"
#include "image_frame.h"
#include "texture.h"
#include "texture_data_loader.h"

#include "stl/vector.h"
#include "stl/unordered_set.h"
#include "stl/unordered_map.h"
#include "stl/utility.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace engine::rhi::runtime {
    class Rhi;
    class Device;
}

namespace engine::graphics {
struct ImageHolder {
    ImageHandler * image = nullptr;
    uint32_t count = 0u;
};

struct ImageHolderEqualTo {
    using is_transparent = void;

    constexpr auto operator()(const rhi::runtime::Image * img, const graphics::ImageHolder & holder) const noexcept { 
        return img == holder.image->image.get();
    }
    constexpr auto operator()(const ImageHandler* img, const graphics::ImageHolder & holder) const noexcept { 
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
    std::size_t operator()(const graphics::ImageHandler * image) const { return hash_type{}(image->image.get()); }
    std::size_t operator()(const graphics::ImageHolder & image) const { return hash_type{}(image.image->image.get()); }
};

using ImageHolder_hash = Hasher<graphics::ImageHolder>;
}

namespace engine::graphics {

class TextureManager {
public:
    TextureManager(rhi::runtime::Rhi & api, rhi::runtime::Device & device);
    ~TextureManager() { destroy(); }
    void createDefaultResources(const rhi::runtime::CommandListPtr & commandList);
    void destroy() noexcept;

    const ImageFrame2D & addFrame(std::string_view name, TextureCreateInfo && info, const Frame2D & frame = {});
    const ImageFrame2D & addFrame(std::string_view name, const rhi::runtime::ImagePtr & image, const Frame2D & frame = {});
    void addFrames(stl::span<stl::pair<std::string, Frame2D>> frames, const rhi::runtime::ImagePtr & image);
    bool removeFrame(std::string_view name) noexcept;
    
    const rhi::runtime::ImagePtr & getImage(uint32_t idx) const noexcept;
    const ImageFrame2D & findFrame(std::string_view name) const noexcept;

    const rhi::runtime::ImagePtr & getDefaultImage() const noexcept { return _defaultImage; }
    const rhi::runtime::ImagePtr & getWhiteImage() const noexcept { return _whiteImage; }
    const rhi::runtime::ImagePtr & getBlackImage() const noexcept { return _blackImage; }
    const rhi::runtime::ImagePtr & getTransparentImage() const noexcept { return _transparentImage; }
    const rhi::runtime::ImagePtr & getDefaultCubeMapImage() const noexcept { return _defaultCubeMapImage; }

    void generateGpuData(rhi::runtime::CommandListPtr & commandList);

    const rhi::runtime::Device & getDevice() const noexcept { return _device; }

    stl::pair<ImageHandler *, bool> addImage(std::string_view name);
    ImageHandler * getImageHandler(std::string_view name) noexcept; // move it to private
    ImageHandler * getDefaultImageHandler() noexcept { return _images[0u].get(); }
    ImageHandler * getWhiteImageHandler() noexcept { return _images[1u].get(); }
    ImageHandler * getBlackImageHandler() noexcept { return _images[2u].get(); }
    ImageHandler * getTransparentImageHandler() noexcept { return _images[3u].get(); }
    ImageHandler * getDefaultCubeImageHandler() noexcept { return _images[4u].get(); }

    rhi::runtime::ImagePtr generateImage(TextureCreateInfo && info, rhi::runtime::CommandList & commandList) const;
    rhi::runtime::ImagePtr generateImage(rhi::Format format, const vec3u & size, TextureFlags flags, stl::span<TextureLayerData> layersData, 
        rhi::ImageCreateFlags createFlags, rhi::runtime::CommandList & commandList) const;

private:
    using ImageFramesMap = stl::unordered_map<std::string, ImageFrame2D, hash::String_hash, std::equal_to<>>;
    using Images = stl::vector<ImageHandlerPtr>;
    using ImagesSet = stl::unordered_set<ImageHolder, hash::ImageHolder_hash, graphics::ImageHolderEqualTo>;

    ImageHandlerPtr & getImageHandler(uint32_t idx) noexcept;

    rhi::runtime::Rhi & _api;
    rhi::runtime::Device & _device;

    rhi::runtime::ImagePtr _defaultImage;
    rhi::runtime::ImagePtr _whiteImage;
    rhi::runtime::ImagePtr _blackImage;
    rhi::runtime::ImagePtr _transparentImage;
    rhi::runtime::ImagePtr _defaultCubeMapImage;

    ImageFrame2D _defaultFrame;

    Images _images;
    ImagesSet _imagesSet;
    ImageFramesMap _frames2D;

    stl::vector<stl::pair<rhi::runtime::ImagePtr, TextureCreateInfo>> _tmpTexturesData;
};

}