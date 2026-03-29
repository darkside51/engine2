#pragma once

#include "graphics/scene/object.h"
#include "graphics/texture/image_frame.h"

#include "math/math.h"

#include <cstdint>
#include <limits>

namespace engine::scene {

    enum class SpriteTextureFilter: uint8_t {
        eNearest = 0u,
        eLinear = 1u
    };

    class Sprite : public AttachmentObject {
    public:
        Sprite(const graphics::ImageFrame2D & frame, uint32_t pipeline);
        ~Sprite() /*override*/ = default;

        void setPipeline(uint32_t pipeline) noexcept;
        uint32_t getPipeline() const noexcept;

        void setFrame(const graphics::ImageFrame2D & frame) noexcept;
        const graphics::ImageFrame2D & getFrame() const noexcept;

        bool cmp(const Sprite &) const noexcept;

        vec2f calculateSize() const noexcept;

        void setSize(vec2f size) noexcept { _size = size; }
        vec2f getSize() const noexcept;

        void attachToNode(Node * node) noexcept;

        void setFilter(SpriteTextureFilter f) noexcept { _filter = f; }
        SpriteTextureFilter getFilter() const noexcept { return _filter; }

    private:
        graphics::ImageFrame2D _frame;
        uint32_t _pipeline = 0u;
        vec2f _size = vec2f{std::numeric_limits<float>::max()};
        SpriteTextureFilter _filter = SpriteTextureFilter::eLinear;
    };

}