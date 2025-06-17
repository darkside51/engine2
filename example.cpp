#include "example.h"

#include "file/common_file_system.h"
#include "graphics/render/rhi/vulkan/runtime.h"
#include "graphics/render/rhi/common/command_list.h"

#include <imgui.h>

namespace example {
    constexpr uint8_t kScreenRenderPass = 0u;
    constexpr uint8_t kImguiProgram = 0u;

    Example::Example() {
        using namespace engine;
        _fileManager.createFileSystem<CommonFileSystem>("common_fs0", CommonFileSystem::getCurrentDirectory() + "./resources/");

        addInputObserver(&_imgui);
    }

    void Example::destroy() {
        if (_renderQueue) {
            _renderQueue->waitIdle();
        }
        if (_device) {
            _device->waitIdle();
        }

        for (auto & semaphore : _readyToPresentSemaphores) {
            _api->destroySemaphore(_device, semaphore);
        }

        _shaderModulesManager = nullptr;
        _imgui.destroy();

        _programs.clear();
        _mainFrameBuffers.clear();
        _renderPasses.clear();
        _depthImages.clear();
        _swapchain = nullptr;

        // _commandLists.clear();
        // _device = nullptr;
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
            .apiVersion = engine::common::Version{1, 3, 0},
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

    void Example::sizeChanged() {
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

            _mainFrameBuffers.emplace_back(_api->createFrameBuffer(_device, std::move(fbAttachemnts), _renderPasses[kScreenRenderPass]));
        }
    }

    void Example::resize(uint16_t w, uint16_t h) {
        if (_renderQueue) {
            _renderQueue->waitIdle();
        }
        if (_device) {
            _device->waitIdle();
        }

        _swapchain->resize(w, h, false);
        
    }

    engine::rhi::RenderPassDescription createRenderPassDescription2(engine::rhi::Format colorFormat, engine::rhi::Format depthFormat) {
        std::vector<engine::rhi::AttachmentDescription> attachments(2u);
        attachments[0].format = colorFormat;
        attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
        attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
        attachments[0].finalLayout = engine::rhi::ImageLayout::ePresent_Src;

        attachments[1].format = depthFormat;
        attachments[1].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eDont_Care}; // or must use eStore?
        attachments[1].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
        attachments[1].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;

        std::vector<engine::rhi::SubpassDescription> subpasses(2u);
        subpasses[0].depthStencilAttachment = {
            .pNext = {nullptr, 0u},
            .attachment = 1u,
            .layout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal,
            .aspectMask = engine::rhi::ImageAspectFlags::eDepthStencil
        };
        subpasses[1].colorAttachments = {{
            .pNext = {nullptr, 0u},
            .attachment = 0u,
            .layout = engine::rhi::ImageLayout::eColor_Attachment_Optimal,
            .aspectMask = engine::rhi::ImageAspectFlags::eColor
        }};
        subpasses[1].depthStencilAttachment = {
            .pNext = {nullptr, 0u},
            .attachment = 1u,
            .layout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal,
            .aspectMask = engine::rhi::ImageAspectFlags::eDepthStencil
        };

        std::vector<engine::rhi::SubpassDependency> deps(1u);
        deps[0].srcSubpass = 0u;
        deps[0].dstSubpass = 1u;
        deps[0].dependencyFlags = engine::rhi::DependencyFlags::eNone; // ? can eNone
        deps[0].stages = {
            .src = {
                .stage = engine::rhi::PipelineStageFlags::eLate_Fragment_Tests,
                .accessMask = engine::rhi::AccessFlags::eDepth_Stencil_Attachment_Write
            },
            .dst = {
                .stage = engine::rhi::PipelineStageFlags::eEarly_Fragment_Tests,
                .accessMask = engine::rhi::AccessFlags::eDepth_Stencil_Attachment_Read,
            }
        };

        return engine::rhi::RenderPassDescription{
            std::move(attachments), 
            std::move(subpasses), 
            std::move(deps)
        };
    }

    void Example::createRenderPasses() {
        const auto depthFormat = _device->getSupportedDepthFormat(32u, false);
        _renderPasses.emplace_back(_api->getRenderPass(_device, createRenderPassDescription2(_swapchain->getFormat(), depthFormat)));
    }

    void Example::loadShaders() {
        _shaderModulesManager = _api->createShaderModulesManager();

        {
            auto const vBytes = _fileManager.readFile("shaders/imgui.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/imgui.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("imgui.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("imgui.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
    }
    
    void Example::createPipelines() {
        loadShaders();
        _imgui.createPipeline(_api, _device, _programs[kImguiProgram], _renderPasses[kScreenRenderPass], 1u);
    }

    void Example::prepare() {
        const auto inflightFrames = _device->inFlightFramesCount();
        _renderQueue = _api->getQueue(_device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, inflightFrames);

        _readyToPresentSemaphores.resize(inflightFrames);
        for (auto & semaphore : _readyToPresentSemaphores) {
            semaphore = _api->createSemaphore(_device);
        }

        _frameIndex.setFramesCount(inflightFrames);

        _imgui.createFontTexture(_api, _device);
        _imgui.createBuffers(_api, _device);

        // on render thread
        auto * commandPool = _device->acquireCommandPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
        _commandLists.reserve(inflightFrames);
        for (size_t i = 0u; i < inflightFrames; ++i) {
            _commandLists.emplace_back(_api->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary));
        }
    }

    void Example::draw(float delta) {
        if (_commandLists.empty()) {
            return;
        }

        auto const size = _swapchain->getSize();
        const auto width = size.width;
        const auto height = size.height;

        _imgui.beginFrame(delta, width, height);
        ImGui::ShowDemoWindow();
        _imgui.endFrame();

        const auto frameId = _frameIndex();

        if (_canDraw) {
            auto const waitSubmitResult = _renderQueue->waitSubmit(frameId); // result!!!
        }

        const auto [imageIndex, success] = _swapchain->acquireNextImage(frameId); // https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation

        if (!success) {
            _canDraw = false;
            return;
        }

        _canDraw = true;
        _device->resetFrameTmpData(frameId);

        // render frame
        auto & commandList = _commandLists[frameId];
        commandList->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        _imgui.prepare(commandList, frameId, _api, _device);

        constexpr auto clearValues = std::array{
            engine::rhi::ClearValue{0x77'77'77'ffu},
            engine::rhi::ClearValue{1.0f}
        };

        commandList->beginRenderPass(_renderPasses[kScreenRenderPass], _mainFrameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}});

        commandList->setViewport(0.0f, 0.0f, width, height);
        commandList->setScissor({{0, 0}, {width, height}});

        commandList->nextSubpass(engine::rhi::SubpassContents::ePrimaryBufferInline);

        _imgui.draw(commandList, frameId);

        commandList->endRenderPass();
        commandList->end();

        commandList->record(frameId);
        // render frame

        // submit
        const auto waitSemaphores = std::array{_swapchain->imageAvailableSemaphore(frameId)};
        const auto signalSemaphores = std::array{_readyToPresentSemaphores[frameId]};
        const auto waitStages = std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output};
        auto lists = std::array{commandList.get()};

        bool const submitted = _renderQueue->submit(frameId, std::array{
            engine::rhi::runtime::Queue::SubmitInfo {
                .w8Semaphores = waitSemaphores,
                .w8DstStages = waitStages,
                .signalSemaphores = signalSemaphores,
                .lists = lists
            }
        });

        ++_frameIndex;

        // present
        auto const presentResult = _renderQueue->present(_swapchain.get(), imageIndex, signalSemaphores);
    }

}