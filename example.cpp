#include "example.h"

#include "file/common_file_system.h"
#include "graphics/render/rhi/vulkan/runtime.h"

namespace example {

    Example::Example() {
        using namespace engine;
        _fileManager.createFileSystem<CommonFileSystem>("common_fs0", CommonFileSystem::getCurrentDirectory() + "./resources/");
    }

    void Example::createApi() {
        using namespace engine;
        using Api = vulkan::Api;

        #ifndef NDEBUG
            bool const kUseApiDebugMessages = true;
            std::vector<std::string_view> extensions = {"VK_KHR_surface", "VK_KHR_win32_surface", 
                "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
                "VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface", 
                Api::kDebugUtilExtName // for validation
            };
            std::vector<std::string_view> layers = {Api::kLayerValidationKHR};
        #else
            bool const kUseApiDebugMessages = false;
            std::vector<std::string_view> extensions = {"VK_KHR_surface", "VK_KHR_win32_surface", 
                "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
                "VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface"
            };
            std::vector<std::string_view> layers = {};
        #endif

        rhi::InstanceCreateInfo instanceCreateInfo {
            .apiVersion = engine::common::Version{1, 0, 0},
            .extensions = extensions,
            .layers = layers,
            .createDebugMessages = kUseApiDebugMessages,
            .allocationCallback = nullptr
        };

        _api = std::make_unique<vulkan::runtime::Rhi>(instanceCreateInfo);
    }

    bool Example::createSurface(const engine::rhi::SurfaceCreateInfo & surfaceInfo) {
        engine::rhi::PhysicalDeviceFeatureRequest gpuFeatures{};
        auto gpuDevices = _api->enumerateDevices({
            gpuFeatures, 
            {
                engine::vulkan::runtime::Rhi::Api::Impl::kSwapchainExtName, 
                VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
            }
        });

        if (gpuDevices.empty()) {
            return false;
        }

        _surface = _api->createSurface(surfaceInfo);

        for (auto & gpuDevice : gpuDevices) {
            if (gpuDevice->checkSurfaceSupport(*_surface)) {
                _gpu = std::move(gpuDevice);
                break;
            }
        }

        if (!_gpu) {
            return false;
        }

        return true;
    }

    bool Example::createDevice() {
        return true;
    }

}