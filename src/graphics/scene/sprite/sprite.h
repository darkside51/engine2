#pragma once

#include "graphics/scene/object.h"
#include "graphics/texture/image_frame.h"

#include <cstdint>

namespace engine::scene {

    class Sprite : public Object {
    public:
        explicit Sprite(const graphics::ImageFrame2D & frame, uint32_t pipeline);
        ~Sprite() = default;

        void setPipeline(uint32_t pipeline) noexcept;
        uint32_t getPipeline() const noexcept;

        void setFrame(const graphics::ImageFrame2D & frame) noexcept;
        const graphics::ImageFrame2D & getFrame() const noexcept;

        bool cmp(const Sprite &) const noexcept;

    private:
        graphics::ImageFrame2D _frame;
        uint32_t _pipeline = 0u;
    };

}