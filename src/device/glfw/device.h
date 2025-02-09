#pragma once


#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

struct GLFWwindow;

namespace engine {

namespace rhi {
    struct SurfaceCreateInfo;
}

    class GLFWDevice {
        friend class GLFWModule;
    public:
        GLFWDevice(std::string_view name);
        ~GLFWDevice();

		void close();

        GLFWDevice(GLFWDevice&& d) noexcept;
        GLFWDevice& operator=(GLFWDevice&& d) noexcept;

        bool operator==(const GLFWDevice & d) noexcept {
            return _window == d._window;
        }

        rhi::SurfaceCreateInfo getSurfaceCreateInfo() const noexcept;

    private:
        GLFWwindow* _window = nullptr;
		uint16_t _width = 0u;
		uint16_t _height = 0u;
		bool _fullscreen = false;
    };

    class GLFWModule {
    public:
        GLFWModule();
        ~GLFWModule();

        GLFWDevice & open(std::string_view name);

        void process(float sleepTime);
        void stop();

    private:
        std::vector<std::unique_ptr<GLFWDevice>> _devices;
    };
}