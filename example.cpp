#include "example.h"

#include "threads/executor.h"
#include "file/common_file_system.h"
#include "graphics/render/rhi/vulkan/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include "graphics/render/staging_buffer.h"

#include "math/math.h"

#include "graphics/scene/camera_controller.h"

#include <imgui.h>

// sail test
#include <sail-c++/sail-c++.h>

// https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/

namespace example {
    constexpr uint8_t kScreenRenderPass = 0u;
    constexpr uint8_t kSceneRenderPass = 1u;

    constexpr uint8_t kImguiProgram = 0u;
    constexpr uint8_t kPostEffectProgram = 1u;
    constexpr uint8_t kTestProgram = 2u;

    constexpr uint8_t kPostEffectPipeline = 0u;
    constexpr uint8_t kSceneTestPipeline = 1u;

    engine::scene::Camera camera;
    engine::scene::SphereCameraController controller{camera};
    engine::mat4f cameraTransform;
    float m = 0.0f;

    struct UniformBuffer {
        engine::mat4f mvp;
        float m;
    };

    class SceneEventHandler final : public engine::InputObserver {
        engine::vec3f v0{0.0f};
        engine::vec3f v1{0.0f};
    public:
        bool onInputPointerEvent(const engine::PointerEvent & event) override {
            static float x0 = 0.0f;
            static float y0 = 0.0f;
            static bool pressed = false;

            switch (event.state) {
            case engine::InputEventState::ePress:
            {
                pressed = true;
            }
                break;
            case engine::InputEventState::eRelease:
            {
                pressed = false;
            }
                break;
            case engine::InputEventState::eNone:
            {
                if (pressed) {
                    
                    v1 += engine::vec3f(0.01f * (y0 - event.y), 0.0f, 0.01f * (event.x - x0));
                    controller.setRotation(
                        v0 + v1
                    );
                }

                x0 = event.x;
                y0 = event.y;
            }
                break;
            default:
                break;
            }

            return true;
        }

        bool onInputWheelEvent(float /*dx*/, float dy) override {
            {
                const auto d = controller.getDistance();
                controller.setDistance(d - (dy / std::pow(2.0f, (8.0f - d))));

                const float lerpBegin = 2.0f;
                const float lerpEnd = 1.0f;
                const float lerpDisatnce = (lerpBegin - lerpEnd);
                const float mix = std::clamp(lerpBegin - d, 0.0f, lerpDisatnce) / lerpDisatnce;
                m = std::lerp(0.0f, 1.0f, mix);

                v0 = engine::vec3f(1.57f * m, 0.0f, 0.0f);

                controller.setRotation(
                    v0 + v1
                );
            }

            // m = std::clamp(m + dy * 0.01f, 0.0f, 1.0f);
            return true;
        }

        bool onInpuKeyEvent(const engine::KeyEvent & event, const engine::InputKeyMod & mod) override {
            return false;
        }

        bool onInpuCharEvent(uint16_t code) override { 
            return false;
        }
    } sceneEventHandler;

    Example::Example() {
        using namespace engine;
        _fileManager.createFileSystem<CommonFileSystem>("common_fs0", CommonFileSystem::getCurrentDirectory() + "./resources/");

        addInputObserver(&_imgui);
        addInputObserver(&sceneEventHandler);

        controller.setDistance(5.0f);
        controller.setMinMaxDistance(0.01f, 8.0f);

        camera.makePerspective(std::numbers::pi_v<float> / 3.0f, 1024.0f / 768.0f, 0.001f, 10.0f);
    }

    Example::~Example() = default;

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

        _commandLists.clear();

        _shaderModulesManager = nullptr;
        _imgui.destroy();

        _pipelines.clear();
        _programs.clear();
        _mainFrameBuffers.clear();
        _renderPasses.clear();
        _depthImages.clear();
        _swapchain = nullptr;
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
        // auto const vkVersion = VK_HEADER_VERSION_COMPLETE; // version from vulkan_core.h
        // engine::common::Version{VK_VERSION_MAJOR(vkVersion), VK_VERSION_MINOR(vkVersion), VK_VERSION_PATCH(vkVersion)};

        rhi::InstanceCreateInfo instanceCreateInfo {
            .apiVersion = engine::common::Version{1, 1, 0}, // need no use PhysicalDeviceFeatures2 to set 1.0.0
            .extensions = extensions,
            .layers = layers,
            .createDebugMessages = kUseApiDebugMessages,
            .allocationCallback = nullptr
        };

        _api = std::make_unique<vulkan::runtime::Rhi>(instanceCreateInfo);
    }

    bool Example::createSurface(const engine::rhi::SurfaceCreateInfo & surfaceInfo) {
        _gpuFeatures = {};
        _gpuFeatures.features.fillModeNonSolid = true;
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

        //
        auto colorImageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eColor_Attachment | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR8G8B8A8_Unorm, {size.width, size.height, 1u}
        }; 
        colorImageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        _colorAttachmentImages.clear();
        _colorAttachmentImages.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            _colorAttachmentImages.emplace_back(std::move(_api->createImage(_device, colorImageInfo)));
        }

        _sceneFrameBuffers.clear();
        _sceneFrameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};

            auto & colorImage = _colorAttachmentImages[i];
            fbAttachemnts[0u].image = colorImage.get();
            fbAttachemnts[0u].imageSize = {colorImage->getExtent().width, colorImage->getExtent().height};
            fbAttachemnts[0u].aspect = engine::rhi::ImageAspectFlags::eColor;  // can get it from colorImage?
            fbAttachemnts[0u].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;

            auto & depthImage = _depthImages[i];
            fbAttachemnts[1u].image = depthImage.get();
            fbAttachemnts[1u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
            fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?
            fbAttachemnts[1u].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

            _sceneFrameBuffers.emplace_back(_api->createFrameBuffer(_device, std::move(fbAttachemnts), _renderPasses[kSceneRenderPass]));
        }

        _mainFrameBuffers.clear();
        _mainFrameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            auto & image = _swapchain->getImage(i);

            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{1u};
            fbAttachemnts[0u].image = &image;
            fbAttachemnts[0u].imageSize = {image.getExtent().width, image.getExtent().height};
            fbAttachemnts[0u].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

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
        camera.resize(w, h);
    }

    engine::rhi::RenderPassDescription createScreenRenderPassDescription(
        engine::rhi::Format swapchainColorFormat
    ) {
        std::vector<engine::rhi::AttachmentDescription> attachments(1u);
        attachments[0].format = swapchainColorFormat;
        attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
        attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
        attachments[0].finalLayout = engine::rhi::ImageLayout::ePresent_Src;

        std::vector<engine::rhi::SubpassDescription> subpasses(1u);
        subpasses[0].colorAttachments = {{
            .pNext = {nullptr, 0u},
            .attachment = 0u,
            .layout = engine::rhi::ImageLayout::eColor_Attachment_Optimal,
            .aspectMask = engine::rhi::ImageAspectFlags::eColor
        }};

        return engine::rhi::RenderPassDescription{
            std::move(attachments), 
            std::move(subpasses), 
            {}
        };
    }

    engine::rhi::RenderPassDescription createSceneRenderPassDescription(
        engine::rhi::Format colorFormat, 
        engine::rhi::Format depthFormat
    ) {
        std::vector<engine::rhi::AttachmentDescription> attachments(2u);
        attachments[0].format = colorFormat;
        attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
        attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
        attachments[0].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;

        attachments[1].format = depthFormat;
        attachments[1].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eDont_Care}; // or must use eStore?
        attachments[1].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
        attachments[1].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;

        std::vector<engine::rhi::SubpassDescription> subpasses(1u);

        subpasses[0].colorAttachments = {{
            .pNext = {nullptr, 0u},
            .attachment = 0u,
            .layout = engine::rhi::ImageLayout::eColor_Attachment_Optimal,
            .aspectMask = engine::rhi::ImageAspectFlags::eColor
        }};
        subpasses[0].depthStencilAttachment = {
            .pNext = {nullptr, 0u},
            .attachment = 1u,
            .layout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal,
            .aspectMask = engine::rhi::ImageAspectFlags::eDepthStencil
        };

        return engine::rhi::RenderPassDescription{
            std::move(attachments), 
            std::move(subpasses), 
            {}
        };
    }

    engine::rhi::RenderPassDescription createSceneRenderPassDescriptionWithDepthPrepass(
        engine::rhi::Format colorFormat, 
        engine::rhi::Format depthFormat
    ) {
        std::vector<engine::rhi::AttachmentDescription> attachments(2u);
        attachments[0].format = colorFormat;
        attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
        attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
        attachments[0].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;

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
        deps[0].dependencyFlags = engine::rhi::DependencyFlags::eBy_Region;
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

        _renderPasses.emplace_back(_api->getRenderPass(_device, createScreenRenderPassDescription(_swapchain->getFormat())));
        _renderPasses.emplace_back(_api->getRenderPass(_device, createSceneRenderPassDescription(engine::rhi::Format::eR8G8B8A8_Unorm, depthFormat)));
        // _renderPasses.emplace_back(_api->getRenderPass(_device, createSceneRenderPassDescriptionWithDepthPrepass(engine::rhi::Format::eR8G8B8A8_Unorm, depthFormat)));
    }

    void Example::loadShaders() {
        _shaderModulesManager = _api->createShaderModulesManager();

        { // kImguiProgram
            auto const vBytes = _fileManager.readFile("shaders/imgui.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/imgui.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("imgui.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("imgui.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
        { // kPostEffectProgram
            auto const vBytes = _fileManager.readFile("shaders/postEffect.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/postEffect.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("postEffect.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("postEffect.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
        { // kTestProgram
            auto const vBytes = _fileManager.readFile("shaders/test.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/test.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("test.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("test.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
    }
    
    void Example::createPipelines() {
        using namespace engine::rhi;

        loadShaders();
        _imgui.createPipeline(_api, _device, _programs[kImguiProgram], _renderPasses[kScreenRenderPass], 0u);

        GraphicsPipelineStatesInfo info;
        info.viewportState = {1u, 1u};
        info.blendModes = {common_blend_modes::blend_none};

        info.depthStencilState = {
            {
                .depthTestEnable = false,
                .depthWriteEnable = false,
                .depthCompareOp = CompareOp::eAlways
            }, 
            {}
        };

        info.rasterisationState = {PolygonMode::eFill, CullMode::eBack};
        info.inputAssemblyState = {PrimitiveTopology::eTriangles, false};
        info.vertexInputState = {
            std::vector{VertexInputBinding::create<float[2]>(0u)},
            std::vector{
                VertexInputAttribute{
                    .location = 0u,
                    .binding = 0u,
                    .format = Format::eR32G32_Sfloat,
                    .offset = 0u
                }
            }
        };

        _pipelines.emplace_back(_api->createGraphicsPipeline(_device, _renderPasses[kScreenRenderPass], _programs[kPostEffectProgram], info, 0u));

        info.rasterisationState = {PolygonMode::eFill, CullMode::eNone};
        info.blendModes = {common_blend_modes::blend_none};
        info.depthStencilState = {
            {
                .depthTestEnable = true,
                .depthWriteEnable = true,
                .depthCompareOp = CompareOp::eLess
            }, 
            {}
        };

        info.vertexInputState = {
            std::vector{VertexInputBinding::create<float[5]>(0u)},
            std::vector{
                VertexInputAttribute{
                    .location = 0u,
                    .binding = 0u,
                    .format = Format::eR32G32B32_Sfloat,
                    .offset = 0u
                },
                VertexInputAttribute{
                    .location = 1u,
                    .binding = 0u,
                    .format = Format::eR32G32_Sfloat,
                    .offset = 12u
                }
            }
        };

        _pipelines.emplace_back(_api->createGraphicsPipeline(_device, _renderPasses[kSceneRenderPass], _programs[kTestProgram], info, 0u));
    }

    void Example::createDefaultResources() {
        // sail test
        auto const imageBytes = _fileManager.readFile("assets/earth.webp");
        sail::image_input imageInput(imageBytes.data(), imageBytes.size());

        sail::image image = imageInput.next_frame();

        // auto [image, codecInfo] = imageInput.probe(); // get only information
        
        const auto format = image.pixel_format(); // need to check for support format on gpu device for target VkFormatFeatureFlagBits !!!!
        const auto bpp = image.bits_per_pixel();
        const auto size = engine::vec2u(image.width(), image.height());
        const auto pixels = image.pixels();
        const auto psz = image.pixels_size();
        const bool vaild = image.is_valid();

        const auto featureAccess24 = _device->isFormatSupported(engine::rhi::Format::eR8G8B8_Unorm, 
            engine::rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image);
        const auto featureAccess16 = _device->isFormatSupported(engine::rhi::Format::eR4G4B4A4_Unorm_Pack16, 
            engine::rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image);

        if (bpp == 24 && featureAccess24.optimalTilingFeatures == false && featureAccess24.linearTilingFeatures == false) {
            // if (featureAccess16.optimalTilingFeatures || featureAccess16.linearTilingFeatures) {
            //     auto status = image.convert(SAIL_PIXEL_FORMAT_BPP16_RGBA);
            //     const auto bpp2 = image.bits_per_pixel();
            // } else {
                auto status = image.convert(SAIL_PIXEL_FORMAT_BPP32_RGBA);
                const auto bpp2 = image.bits_per_pixel();
            // }
        }


        // create post buffers and default image
        using namespace engine::rhi;
        using namespace engine::graphics;

        BufferInfo bufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        _postPositions = _api->createBuffer(_device, bufferInfo, sizeof(std::array<float, 2u>) * 4u);

        bufferInfo.usage = BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst;
        _postIndices = _api->createBuffer(_device, bufferInfo, sizeof(uint16_t) * 6u);

        const auto imageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {2u, 2u, 1u}
        };

        _defaultImage = _api->createImage(_device, imageInfo);

        // staging buffer for vtx, idx, image;
        constexpr auto stagingSize = sizeof(std::array<float, 2u>) * 4u + sizeof(uint16_t) * 6u + sizeof(uint32_t) * 4u;
        StagingBuffer staging{_api,_device, stagingSize};
        
        std::byte * memory = staging.map();
        const auto vtxOffset = staging.requestBytes(sizeof(std::array<float, 2u>) * 4u);
        const auto idxOffset = staging.requestBytes(sizeof(uint16_t) * 6u);
        const auto imgOffset = staging.requestBytes(sizeof(uint32_t) * 4u);

        auto * vtxData = reinterpret_cast<std::array<float, 2u>*>(memory + vtxOffset);
        auto * idxData = reinterpret_cast<uint16_t*>(memory + idxOffset);
        auto * imgData = reinterpret_cast<uint32_t*>(memory + imgOffset);

        vtxData[0] = {-1.0, -1.0}; vtxData[1] = {-1.0, 1.0};
        vtxData[2] = {1.0, 1.0}; vtxData[3] = {1.0, -1.0}; 
        
        idxData[0] = 0u; idxData[1] = 1u; idxData[2] = 2u;
        idxData[3] = 0u; idxData[4] = 2u; idxData[5] = 3u;

        imgData[0] = 0xff'ff'ff'ffu; imgData[1] = 0xff'55'55'55u;
        imgData[2] = 0xff'55'55'55u; imgData[3] = 0xff'ff'ff'ffu;

        staging.flush();
        staging.unmap();
        // staging buffer works complete

        // sphere data
        using namespace engine;
        uint16_t sectorCount = 32;
        uint16_t stackCount = 32;
        float radius = 1.0f;

        constexpr float pi = std::numbers::pi_v<float>;

        const float sphereWidthPart = 1.0f;
        const float sphereHeightPart = 1.0f;

        const float sectorStep = 2.0f * pi / sectorCount * sphereWidthPart;

        const float stackFullAngle = pi;// - 1.5f;
        const float startAngle = stackFullAngle / 2.0f;
        const float stackStep = stackFullAngle / stackCount * sphereHeightPart;
        const bool truncatedSphere = std::abs(stackFullAngle - pi) > 1e-5f;

        std::vector<std::array<float, 5u>> vertices;
        std::vector<uint16_t> indices;

        // vtx
        for(uint16_t i = 0u; i <= stackCount; ++i) {
            const float stackAngle = startAngle - i * stackStep; // starting from startAngle to startAngle-stackFullAngle
        
            // const float xy = radius * std::cos(stackAngle); // r * cos(u)
            // const float z = radius * std::sin(stackAngle);  // r * sin(u)

            for(int16_t j = -sectorCount / 2; j <= sectorCount / 2; ++j) {
                const float sectorAngle = j * sectorStep; // starting from 0 to 2pi

                // vertex position (x, y, z)
                // const float x = xy * std::cos(sectorAngle); // r * cos(u) * cos(v)
                // const float y = xy * std::sin(sectorAngle); // r * cos(u) * sin(v)
                const float u = static_cast<float>(j + sectorCount / 2) / sectorCount;
                const float v = static_cast<float>(i) / stackCount;
                // vertices.emplace_back(std::array{x, y, z, u, v});
                vertices.emplace_back(std::array{stackAngle, sectorAngle, 0.0f, u, v});
            }
        }

        // idx
        for(uint16_t i = 0u; i < stackCount; ++i) {
            auto k1 = i * (sectorCount + 1); // beginning of current stack
            auto k2 = k1 + sectorCount + 1;  // beginning of next stack

            for(uint16_t j = 0u; j < sectorCount; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1 + 1
                //if (truncatedSphere || (i != 0u)) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                //}

                // k1 + 1 => k2 => k2 + 1
                //if (truncatedSphere || (i != (stackCount - 1))) {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                //}
            }
        }

        const auto vtxDataSize = sizeof(std::array<float, 5u>) * vertices.size();
        const auto idxDataSize = sizeof(uint16_t) * indices.size();

        const auto stagingSphereSize = vtxDataSize + idxDataSize;
        StagingBuffer stagingSphere{_api,_device, stagingSphereSize};
        std::byte * memory2 = stagingSphere.map();
        const auto vtxOffset2 = stagingSphere.requestBytes(vtxDataSize);
        const auto idxOffset2 = stagingSphere.requestBytes(idxDataSize);

        auto * vtxData2 = reinterpret_cast<std::array<float, 5u>*>(memory2 + vtxOffset2);
        auto * idxData2 = reinterpret_cast<uint16_t*>(memory2 + idxOffset2);

        std::memcpy(vtxData2, vertices.data(), vtxDataSize);
        std::memcpy(idxData2, indices.data(), idxDataSize);

        stagingSphere.flush();
        stagingSphere.unmap();

        BufferInfo bufferInfoSphere {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };
        _sphereVtx = _api->createBuffer(_device, bufferInfoSphere, sizeof(std::array<float, 5u>) * vertices.size());

        bufferInfoSphere.usage = BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst;
        _sphereIdx = _api->createBuffer(_device, bufferInfoSphere, sizeof(uint16_t) * indices.size());
        // sphere data

        // sphere texture
        const auto sphereImageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {image.width(), image.height(), 1u}
        };

        _sphereImage = _api->createImage(_device, sphereImageInfo);
        const auto stagingSize3 = image.pixels_size();
        StagingBuffer staging3{_api,_device, stagingSize3};
        
        std::byte * memory3 = staging3.map();
        auto * sphereImgData = reinterpret_cast<std::byte*>(memory3);
        std::memcpy(memory3, image.pixels(), stagingSize3);
        // const auto* sphereTexPixels = static_cast<std::byte*>(image.pixels());
        // for (size_t i = 0; i < stagingSize3; ++i) {
        //     sphereImgData[i] = sphereTexPixels[i];
        // }
        staging3.flush();
        staging3.unmap();
        // sphere texture

        auto * commandPool = _device->acquireCommandPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
        auto tmpCommandList = _api->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary);

        tmpCommandList->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        tmpCommandList->copyBuffer2Buffer(staging.getBuffer(), _postPositions, 
            {.srcOffset = vtxOffset, .dstOffset = 0u, .size = _postPositions->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(staging.getBuffer(), _postIndices, 
            {.srcOffset = idxOffset, .dstOffset = 0u, .size = _postIndices->sizeInBytes()});
        tmpCommandList->copyBuffer2Image(staging.getBuffer(), _defaultImage, 
            {
                .bufferOffset = imgOffset,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _defaultImage->getExtent()
            }
        );

        // sphere
        tmpCommandList->copyBuffer2Buffer(stagingSphere.getBuffer(), _sphereVtx, 
            {.srcOffset = vtxOffset2, .dstOffset = 0u, .size = _sphereVtx->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(stagingSphere.getBuffer(), _sphereIdx, 
            {.srcOffset = idxOffset2, .dstOffset = 0u, .size = _sphereIdx->sizeInBytes()});

        // texture
        tmpCommandList->copyBuffer2Image(staging3.getBuffer(), _sphereImage, 
            {
                .bufferOffset = 0u,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _sphereImage->getExtent()
            }
        );
        // sphere

        tmpCommandList->end();
        tmpCommandList->record(0u);

        auto lists = std::array{tmpCommandList.get()};
        auto fence = _api->createFence(_device, FenceCreateFlags::eNone);
        bool const submitted = _renderQueue->submit(0u, std::array{
            engine::rhi::runtime::Queue::SubmitInfo {
                .w8Semaphores = {},
                .w8DstStages = {},
                .signalSemaphores = {},
                .lists = lists
            }
        }, fence);

        _renderQueue->waitSubmit(0u, UINT64_MAX, fence);
        _api->destroyFence(_device, fence);

        BufferInfo unifromsInfo {
            BufferUsageFlags::eUniform_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal | MemoryPropertyFlags::eHostVisible
        };

        const size_t kUniformBufferOffsetAlign = _device->getUniformBufferOffsetAlignment();
        _uniformBufferI = std::make_unique<engine::graphics::InflightBuffer>(_api, _device, unifromsInfo, std::max(kUniformBufferOffsetAlign, sizeof(UniformBuffer)));
    
        { // update uniforms
            auto * map = _uniformBufferI->map2<UniformBuffer>();
            map->mvp = engine::makeMatrix(1.0f);
            map->m = m;
            _uniformBufferI->unmap();
        }
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

        //
        createDefaultResources();
    }

    void Example::processLogic() {
        if (controller.calculateTransform() && _renderThread) {
            _renderThread->emplaceTasks(stl::make_vector(threads::WorkerTask{[transform = camera.getTransform()](){
                cameraTransform = transform;
            }}));
        }
    }

    void Example::update(float delta) {
        const auto [width, height] = _swapchain->getSize();
        _imgui.beginFrame(delta, width, height);
        ImGui::ShowDemoWindow();
        _imgui.endFrame();
    }

    void Example::prepareFrameDrawData(engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, float delta) {
        using namespace engine;
        { // update uniforms
            auto * map = _uniformBufferI->map2<UniformBuffer>();
            //rotateMatrix_z(*map, -delta);

            map->mvp = cameraTransform;
            map->m = m;

            _uniformBufferI->unmap();
            _uniformBufferI->commit(commandList, frame);
        }

        _imgui.prepare(commandList, frame, _api, _device);
    }

    void Example::draw(float delta) {
        using namespace engine;
        if (_commandLists.empty()) {
            return;
        }

        const auto [width, height] = _swapchain->getSize();
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
        commandList->begin(rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        // prepare frame draw data
        prepareFrameDrawData(commandList, frameId, delta);

        // render
        constexpr auto sceneClearValues = std::array{
            rhi::ClearValue{0x77'77'77'ffu},
            rhi::ClearValue{1.0f}
        };

        constexpr auto clearValues = std::array{
            rhi::ClearValue{0x77'77'77'ffu},
        };

        commandList->setViewport(0.0f, 0.0f, width, height);
        commandList->setScissor({{0, 0}, {width, height}});

        // scene pass
        commandList->beginRenderPass(_renderPasses[kSceneRenderPass], _sceneFrameBuffers[imageIndex], sceneClearValues, {{0, 0}, {width, height}});

        commandList->bindPipeline(_pipelines[kSceneTestPipeline]);
        commandList->bindVertexBuffer(0u, _sphereVtx);
        commandList->bindIndexBuffer(_sphereIdx, 0u, rhi::IndexType::eU16);
        commandList->bindImage(1u, _sphereImage, rhi::SamplerInfo{
            .magFilter = rhi::FilterMode::eLinear,
            .minFilter = rhi::FilterMode::eLinear});
        commandList->bindBuffer(0u, (*_uniformBufferI)(frameId), 0u, 0u, sizeof(UniformBuffer));
        commandList->drawIndexed(_sphereIdx->sizeInBytes() / sizeof(uint16_t));

        //commandList->nextSubpass(rhi::SubpassContents::ePrimaryBufferInline);
        commandList->endRenderPass();

        // final pass
        std::array images2Shaders{rhi::runtime::ImageTransitionInfo{_colorAttachmentImages[imageIndex].get()}};
        commandList->beginRenderPass(_renderPasses[kScreenRenderPass], _mainFrameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}}, images2Shaders);

        // draw with postEffect
        commandList->bindPipeline(_pipelines[kPostEffectPipeline]);
        commandList->bindVertexBuffer(0u, _postPositions);
        commandList->bindIndexBuffer(_postIndices, 0u, rhi::IndexType::eU16);
        commandList->bindImage(0u, _colorAttachmentImages[imageIndex], rhi::SamplerInfo{});
        commandList->drawIndexed(6u);

        // draw imgui
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