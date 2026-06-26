#include "device.h"

#include "graphics/render/rhi/common/surface.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>

namespace engine {
    
    GLFWModule::GLFWModule() {
        glfwInit();
    }

    GLFWModule::~GLFWModule() {
        glfwTerminate();
    }

    GLFWDevice & GLFWModule::createWindow(std::string_view name, uint16_t w, uint16_t h) {
        return *_devices.emplace_back(std::make_unique<GLFWDevice>(name, w, h));
    }

    bool GLFWModule::process() {
        if (!_devices.empty()) {
            glfwPollEvents();

            auto sz = _devices.size();
            for (size_t i = 0u; i < sz;) {
                auto const & device = _devices[i];
                if (glfwWindowShouldClose(device->_window)) {
                    std::swap(_devices[i], _devices[sz - 1]);
                    --sz;
                } else {
                    ++i;
                }
            }

            _devices.erase(_devices.end() - (_devices.size() - sz), _devices.end());
            return true;
        }
        return false;
    }

    void GLFWModule::stop() {
        for (auto & device : _devices) {
            device->close();
        }
    }

    //
    void glfwOnKeyboard(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
        auto * device = static_cast<GLFWDevice*>(glfwGetWindowUserPointer(window));
        device->keyEvent(key, action);
    }

    void glfwOnWindowResize(GLFWwindow* window, int w, int h) {
        auto * device = static_cast<GLFWDevice*>(glfwGetWindowUserPointer(window));
        device->resize(static_cast<uint16_t>(w), static_cast<uint16_t>(h));
	}

    void glfwOnMouseMove(GLFWwindow* window, double x, double y) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
        auto * device = static_cast<GLFWDevice*>(glfwGetWindowUserPointer(window));
        device->pointerEvent(-1, -1, static_cast<float>(x), static_cast<float>(h) - static_cast<float>(y));
	}

	void glfwOnMouseButton(GLFWwindow* window, int button, int action, int /*mods*/) {
		double x, y;
		int w, h;
		glfwGetCursorPos(window, &x, &y);
		glfwGetWindowSize(window, &w, &h);
        auto * device = static_cast<GLFWDevice*>(glfwGetWindowUserPointer(window));
        device->pointerEvent(button, action, static_cast<float>(x), static_cast<float>(h) - static_cast<float>(y));
	}

    void glfwOnScroll(GLFWwindow* window, double xoffset, double yoffset) { // mouse wheel
		auto * device = static_cast<GLFWDevice*>(glfwGetWindowUserPointer(window));
        device->wheel(static_cast<float>(xoffset), static_cast<float>(yoffset));
	}

    void glfwOnDragAndDrop(GLFWwindow* window, int count, const char** paths) {
        auto * device = static_cast<GLFWDevice*>(glfwGetWindowUserPointer(window));
        static std::vector<std::string_view> views;
        views.reserve(count);
        for (int i = 0; i < count; ++i) {
            views.emplace_back(paths[i]);
        }
        device->dragAndDrop(views);
    }

    GLFWDevice::GLFWDevice(GLFWDevice&& d) noexcept {
        _window = d._window;
        _width = d._width;
        _height = d._height;
        _fullscreen = d._fullscreen;
        d._window = nullptr;
    }

    GLFWDevice& GLFWDevice::operator=(GLFWDevice&& d) noexcept {
        if (_window) {
            glfwDestroyWindow(_window);
        }

        _window = d._window;
        _width = d._width;
        _height = d._height;
        _fullscreen = d._fullscreen;
        d._window = nullptr;
        return *this;
    }

    GLFWDevice::GLFWDevice(std::string_view name, uint16_t w, uint16_t h) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		_width = w;
		_height = h;

		_window = glfwCreateWindow(_width, _height, name.data(), nullptr, nullptr);
        glfwSetWindowUserPointer(_window, this);
        
        // //glfwSetFramebufferSizeCallback(_window, &glfwOnFrameBufferResize);
		glfwSetWindowSizeCallback(_window, &glfwOnWindowResize);
		// glfwSetWindowIconifyCallback(_window, &glfwOnWindowIconify);

		// // input callbacks
		glfwSetCursorPosCallback(_window, &glfwOnMouseMove);
		glfwSetMouseButtonCallback(_window, &glfwOnMouseButton);
		glfwSetScrollCallback(_window, &glfwOnScroll);
		glfwSetKeyCallback(_window, &glfwOnKeyboard);
		// glfwSetCharCallback(_window, &glfwOnChar);
        glfwSetDropCallback(_window, glfwOnDragAndDrop);
    }

    GLFWDevice::~GLFWDevice() {
        if (_onClose) {
            _onClose();
        }
        
        if (_window) {
            glfwDestroyWindow(_window);
            _window = nullptr;
        }
    }

    void GLFWDevice::setVisible(bool visible) noexcept {
        if (visible) {
            glfwShowWindow(_window);
        } else {
            glfwHideWindow(_window);
        }
    }

	void GLFWDevice::close() {
        glfwSetWindowShouldClose(_window, 1);
    }

    void GLFWDevice::resize(uint16_t w, uint16_t h) {
        if (_onResize) {
            _onResize(w, h);
        }
    }
    
    void GLFWDevice::pointerEvent(int button, int action, float x, float y) {
        if (_onPointerEvent) {
            if (button != -1) {
                _onPointerEvent(PointerEvent(static_cast<PointerButton>(button), 
                    (action == 0 ? InputEventState::eRelease : InputEventState::ePress),
                    x, y));
            } else {
                _onPointerEvent(PointerEvent(PointerButton::eButtonNone, 
                                   InputEventState::eNone, x, y));
            }
        }
    }

    void GLFWDevice::wheel(float x, float y) {
        if (_onWheelEvent) {
            _onWheelEvent(x, y);
        }
    }

    void GLFWDevice::keyEvent(int key, int action) {
        if (_onKeyEvent) {
            KeyboardKey k;
            uint8_t specialMask = 0u;

            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                k = static_cast<KeyboardKey>((key - GLFW_KEY_0) + static_cast<int>(KeyboardKey::e0));
            } else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
                k = static_cast<KeyboardKey>((key - GLFW_KEY_A) + static_cast<int>(KeyboardKey::eA));
            } else if (key >= GLFW_KEY_ESCAPE && key <= GLFW_KEY_F25) {
                k = static_cast<KeyboardKey>((key - GLFW_KEY_ESCAPE) + static_cast<int>(KeyboardKey::eEscape));
            } else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_MENU) {
                k = static_cast<KeyboardKey>((key - GLFW_KEY_KP_0) + static_cast<int>(KeyboardKey::eKP0));

                switch (k) {
                    case KeyboardKey::eLeftAlt:
                    case KeyboardKey::eRightAlt:
                        specialMask |= InputKeyMod::ALT_PRESSED_BIT;
                        break;
                    case KeyboardKey::eLeftControl:
                    case KeyboardKey::eRightControl:
                        specialMask |= InputKeyMod::CTRL_PRESSED_BIT;
                        break;
                    case KeyboardKey::eLeftShift:
                    case KeyboardKey::eRightShift:
                        specialMask |= InputKeyMod::SHIFT_PRESSED_BIT;
                        break;
                    case KeyboardKey::eLeftSuper:
                    case KeyboardKey::eRightSuper:
                        specialMask |= InputKeyMod::SUPER_PRESSED_BIT;
                        break;
                    default:
                        break;
                }
            } else {
                switch (key) {
                    case GLFW_KEY_BACKSPACE:
                        k = KeyboardKey::eBackSpace;
                        break;
                    case GLFW_KEY_SPACE:
                        k = KeyboardKey::eSpace;
                        break;
                    case GLFW_KEY_APOSTROPHE:
                        k = KeyboardKey::eApostrophe;
                        break;
                    case GLFW_KEY_COMMA:
                        k = KeyboardKey::eComma;
                        break;
                    case GLFW_KEY_MINUS:
                        k = KeyboardKey::eMinus;
                        break;
                    case GLFW_KEY_PERIOD:
                        k = KeyboardKey::ePeriod;
                        break;
                    case GLFW_KEY_SLASH:
                        k = KeyboardKey::eSlash;
                        break;
                    case GLFW_KEY_SEMICOLON:
                        k = KeyboardKey::eSemicolon;
                        break;
                    case GLFW_KEY_EQUAL:
                        k = KeyboardKey::eEqual;
                        break;
                    case GLFW_KEY_LEFT_BRACKET:
                        k = KeyboardKey::eLeftBracket;
                        break;
                    case GLFW_KEY_BACKSLASH:
                        k = KeyboardKey::eBackSlash;
                        break;
                    case GLFW_KEY_RIGHT_BRACKET:
                        k = KeyboardKey::eRightBracket;
                        break;
                    case GLFW_KEY_GRAVE_ACCENT:
                        k = KeyboardKey::eGraveAccent;
                        break;
                    default:
                        k = KeyboardKey::eUnknown;
                        action = 255; // for no call onKeyEvent
                        break;
                }
            }

            switch (action) {
                case GLFW_PRESS:
                    _onKeyEvent({k, InputEventState::ePress}, specialMask);
                    break;
                case GLFW_RELEASE:
                    _onKeyEvent({k, InputEventState::eRelease}, specialMask);
                    break;
                case GLFW_REPEAT:
                    _onKeyEvent({k, InputEventState::eRepeate}, specialMask);
                    break;
                default:
                    break;
            }
        }
    }

    void GLFWDevice::dragAndDrop(std::span<std::string_view> paths) {
        if (_onDragAndDrop) {
            _onDragAndDrop(paths);
        }
    }

    rhi::SurfaceCreateInfo GLFWDevice::getSurfaceCreateInfo() const noexcept {
        #ifdef GLFW_EXPOSE_NATIVE_X11
        return {rhi::SurfaceType::eWindowSurface, glfwGetX11Display(), reinterpret_cast<void*>(glfwGetX11Window(_window))};
        #elif defined GLFW_EXPOSE_NATIVE_WIN32
        return {rhi::SurfaceType::eWindowSurface, nullptr, reinterpret_cast<void*>(glfwGetWin32Window(_window))};
        #endif
    }
}
