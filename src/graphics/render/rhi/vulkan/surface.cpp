#include "surface.h"

#include "instance.h"

#include <any>
#include <cassert>
#include <cstdint>

#if defined VK_USE_PLATFORM_XCB_KHR || defined VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#include <dlfcn.h> // dlopen, dlclose, ...
#elif defined VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif

namespace engine::vulkan {

    VkSurfaceKHR createWindowSurface(const Instance & instance, const rhi::SurfaceCreateInfo & info) {
        VkSurfaceKHR surface;
        bool success = false;
#if defined VK_USE_PLATFORM_WIN32_KHR // windows variant
			VkWin32SurfaceCreateInfoKHR vk_surfaceInfo;
			vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			vk_surfaceInfo.pNext = nullptr;
			vk_surfaceInfo.flags = 0;
			vk_surfaceInfo.hinstance = GetModuleHandle(nullptr);
			vk_surfaceInfo.hwnd = static_cast<HWND>(reinterpret_cast<size_t>(info.window));

			success = vkCreateWin32SurfaceKHR(instance.vkInstance(), &vk_surfaceInfo,
                                           nullptr, &surface) == VK_SUCCESS;
#elif defined VK_USE_PLATFORM_WAYLAND_KHR
            // https://github.com/googlesamples/vulkan-basic-samples/blob/master/Sample-Programs/Hologram/ShellWayland.cpp
			success = false; // todo!
#elif defined VK_USE_PLATFORM_XCB_KHR
            using xcbConnectionGetter = xcb_connection_t* (*)(Display*);
            auto x11handle = dlopen("libX11-xcb.so.1", RTLD_LAZY | RTLD_LOCAL);
            auto getXCBConnection = reinterpret_cast<xcbConnectionGetter>(dlsym(x11handle, "XGetXCBConnection"));
            xcb_connection_t* connection = getXCBConnection(static_cast<Display*>(info.display));
            dlclose(x11handle);

            VkXcbSurfaceCreateInfoKHR vk_surfaceInfo;
            vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            vk_surfaceInfo.pNext = nullptr;
            vk_surfaceInfo.flags = 0;
            vk_surfaceInfo.connection = connection;
            vk_surfaceInfo.window = static_cast<xcb_window_t>(reinterpret_cast<size_t>(info.window));

            success = vkCreateXcbSurfaceKHR(instance.vkInstance(), &vk_surfaceInfo,
                                         nullptr, &surface) == VK_SUCCESS;
#elif defined VK_USE_PLATFORM_XLIB_KHR
            VkXlibSurfaceCreateInfoKHR vk_surfaceInfo;
            vk_surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            vk_surfaceInfo.pNext = nullptr;
            vk_surfaceInfo.flags = 0;
            vk_surfaceInfo.dpy = static_cast<Display*>(info.display);
            vk_surfaceInfo.window = static_cast<Window>(reinterpret_cast<size_t>(info.window));

			success = vkCreateXlibSurfaceKHR(instance.vkInstance(), &vk_surfaceInfo,
                                          nullptr, &surface) == VK_SUCCESS;
#endif

    assert(success);
    return surface;
    }

    Surface::Surface(const Instance & instance, const rhi::SurfaceCreateInfo & info) : _type(info.type) {
        switch(_type) {
            case rhi::SurfaceType::eWindowSurface :
                _surface = createWindowSurface(instance, info);
            break;
            // todo!
            case rhi::SurfaceType::eDisplay :
            break;
            case rhi::SurfaceType::eAndroid :
            break;
        }
    }

}