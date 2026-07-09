#pragma once

#include "input/input.h"
#include "stl/span.h"
#include "stl/vector.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

struct GLFWwindow;

namespace engine {

namespace rhi {
    struct SurfaceCreateInfo;
}

    class GLFWDevice {
        friend class GLFWModule;
    public:
        GLFWDevice(std::string_view name, uint16_t w, uint16_t h);
        ~GLFWDevice();

		void close();
        void resize(uint16_t w, uint16_t h);
        void pointerEvent(int button, int action, float x, float y);
        void wheel(float x, float y);
        void keyEvent(int key, int action);
        void dragAndDrop(stl::span<std::string_view> paths);

        GLFWDevice(GLFWDevice&& d) noexcept;
        GLFWDevice& operator=(GLFWDevice&& d) noexcept;

        bool operator==(const GLFWDevice & d) noexcept {
            return _window == d._window;
        }

        rhi::SurfaceCreateInfo getSurfaceCreateInfo() const noexcept;

        std::pair<uint16_t, uint16_t> getSize() const noexcept {
            return {_width, _height};
        }
        
        void setOnClose(std::function<void()> && callback) noexcept {
            _onClose = std::move(callback);
        }

        void setOnResize(std::function<void(uint16_t w, uint16_t h)> && callback) noexcept {
            _onResize = std::move(callback);
        }

        void setOnPointer(std::function<void(PointerEvent && event)> && callback) noexcept {
            _onPointerEvent = std::move(callback);
        }

        void setOnWheel(std::function<void(float dx, float dy)> && callback) noexcept {
            _onWheelEvent = std::move(callback);
        }

        void setOnKey(std::function<void(KeyEvent && evt, uint8_t mask)> && callback) noexcept {
            _onKeyEvent = std::move(callback);
        }

        void setOnDragAndDrop(std::function<void(stl::span<std::string_view> paths)> && callback) noexcept {
            _onDragAndDrop = std::move(callback);
        }

        void setVisible(bool visible) noexcept;

    private:
        GLFWwindow* _window = nullptr;
		uint16_t _width = 0u;
		uint16_t _height = 0u;
		bool _fullscreen = false;
        bool _visible = true;
        std::function<void()> _onClose{};
        std::function<void(uint16_t w, uint16_t h)> _onResize{};
        std::function<void(PointerEvent && event)> _onPointerEvent{};
        std::function<void(float dx, float dy)> _onWheelEvent{};
        std::function<void(KeyEvent && evt, uint8_t mask)> _onKeyEvent{};
        std::function<void(stl::span<std::string_view> paths)> _onDragAndDrop{};
    };

    class GLFWModule {
    public:
        GLFWModule();
        ~GLFWModule();

        GLFWDevice & createWindow(std::string_view name, uint16_t w, uint16_t h);

        bool process();
        void stop();

    private:
        stl::vector<std::unique_ptr<GLFWDevice>> _devices;
    };
}