#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

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

        void setOnPointer(std::function<void(int button, int action, float x, float y)> && callback) noexcept {
            _onPointerEvent = std::move(callback);
        }

        void setOnWheel(std::function<void(float dx, float dy)> && callback) noexcept {
            _onWheelEvent = std::move(callback);
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
        std::function<void(int button, int action, float x, float y)> _onPointerEvent{};
        std::function<void(float dx, float dy)> _onWheelEvent{};
    };

    class GLFWModule {
    public:
        GLFWModule();
        ~GLFWModule();

        GLFWDevice & createWindow(std::string_view name, uint16_t w, uint16_t h);

        bool process();
        void stop();

    private:
        std::vector<std::unique_ptr<GLFWDevice>> _devices;
    };
}