#include "example.h"

#include "file/common_file_system.h"
#include "graphics/render/rhi/vulkan/runtime.h"

namespace example {
    constexpr uint8_t screenRenderPass = 0u;

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
        _gpuFeatures = {};
        auto gpuDevices = _api->enumerateDevices({
            _gpuFeatures, 
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

    void Example::createDevice() {
        using namespace engine;

        constexpr auto queueFlags = rhi::queue_flags::graphics | rhi::queue_flags::present;
        const auto [index, family] = _gpu->getQueueFamily(queueFlags);

        rhi::DeviceCreateInfo deviceCreateInfo{_gpuFeatures};
        deviceCreateInfo.queueFamilies.push_back({index, 1u, {1.0f}, queueFlags});
        deviceCreateInfo.commandPoolThreadsCount = 1u;
        deviceCreateInfo.inFlightFramesCount = 3u;

        _device = _api->createDevice(_gpu, deviceCreateInfo);
    }

    void Example::createSwapchain() {
        using namespace engine;

        constexpr auto queueFlags = rhi::queue_flags::graphics | rhi::queue_flags::present;
        const auto [index, family] = _gpu->getQueueFamily(queueFlags);
        const uint8_t inflightFrames = _device->inFlightFramesCount();

        rhi::SwapchainCreateInfo swapchainCreateInfo = {};
        swapchainCreateInfo.imagesCount = inflightFrames;
        swapchainCreateInfo.queueFamilies.push_back(index);

        _swapchain = _api->createSwapchain(_device, _surface, swapchainCreateInfo);
    }

    void Example::resize(uint16_t w, uint16_t h) {
        if (_mainQueue) {
            _mainQueue->waitIdle();
        }
        if (_device) {
            _device->waitIdle();
        }

        _swapchain->resize(w, h, false);
        const uint32_t swapchainImagesCount = _swapchain->getImagesCount();

        auto const size = _swapchain->getSize();

        const auto depthFormat = _device->getSupportedDepthFormat(32u, false);

        auto imageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment | engine::rhi::ImageUsageFlags::eTransient_Attachment,
            depthFormat, {size.width, size.height, 1u}
        }; 
        imageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        _depthImages.clear();
        _depthImages.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            _depthImages.emplace_back(std::move(_api->createImage(_device, imageInfo)));
        }

        _mainFrameBuffers.clear();
        _mainFrameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            auto & image = _swapchain->getImage(i);
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};
            fbAttachemnts[0u].image = &image;
            fbAttachemnts[0u].imageSize = {image.getExtent().width, image.getExtent().height};
            fbAttachemnts[0u].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

            auto & depthImage = _depthImages[i];
            fbAttachemnts[1u].image = depthImage.get();
            fbAttachemnts[1u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
            fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?
            fbAttachemnts[1u].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

            _mainFrameBuffers.emplace_back(_api->createFrameBuffer(_device, std::move(fbAttachemnts), _renderPasses[screenRenderPass]));
        }
    }

}