#include "example.h"

#include "threads/executor.h"
#include "file/common_file_system.h"
#include "common/random.h"
#include "graphics/render/rhi/vulkan/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include "graphics/render/staging_buffer.h"

#include "math/math.h"

#include "graphics/scene/camera_controller.h"

#include "graphics/texture/texture_data_loader.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/mesh/gltf_loader.h"
#include "graphics/mesh/mesh_data.h"

#include <imgui.h>

#include "utils/parsers/renderpass_json_parser.h"
#include "utils/parsers/graphics_pipeline_json_parser.h"

#include "common/type_id.h"
#include "graphics/scene/node.h"
#include "graphics/scene/scene.h"
#include "graphics/scene/transform.h"

#include "graphics/debug/debug_draw.h"
#include "graphics/texture/atlas.h"
#include "graphics/scene/animation/texture/frame_sequence_animation.h"
#include "graphics/scene/animation/mesh/skeletal_animation.h"
#include "graphics/scene/animation_manager.h"

// https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
// https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
// https://nikitablack.github.io/post/how_to_use_vulkan_timestamp_queries/

namespace example {
    constexpr uint8_t kScreenRenderPass = 0u;
    constexpr uint8_t kSceneRenderPass = 1u;

    constexpr uint8_t kImguiProgram = 0u;
    constexpr uint8_t kPostEffectProgram = 1u;
    constexpr uint8_t kTestProgram = 2u;
    constexpr uint8_t kComputeProgram = 3u;
    constexpr uint8_t kDebugProgram = 4u;
    constexpr uint8_t kSpritesProgram = 5u;
    constexpr uint8_t kMeshProgram = 6u;
    constexpr uint8_t  kMeshSkeletalInstancedProgram = 7u;
    constexpr uint8_t  kMeshInstancedProgram = 8u;

    constexpr uint8_t kPostEffectPipeline = 0u;
    constexpr uint8_t kSceneTestPipeline = 1u;
    constexpr uint8_t kComputeTestPipeline = 2u;
    constexpr uint8_t kMeshTestPipeline = 3u;

    constexpr bool kUseTransferCommandListForPrepareImgui = true;

    engine::debug::DebugDraw debugDraw;
    engine::graphics::TextureManager textureManager;
    engine::scene::AnimationManager animationManager;

    engine::scene::Camera camera;
    engine::scene::SphereCameraController controller{camera};
    engine::scene::Node * sceneWorldRoot = nullptr;
    engine::scene::Node * sceneNodePtr = nullptr;
    engine::scene::Skeleton * tmpSkeleton = nullptr;
    engine::scene::SkeletalAnimation * skeletalAnimation = nullptr;

    engine::scene::SkeletalAnimator gSkeletalAnimator{};

    engine::mat4f cameraTransform;
    float m = 0.0f;

    engine::scene::Scene scene{&debugDraw};

    struct UniformBuffer {
        engine::mat4f mvp;
        float m;
        float time;
    };

    struct Stats {
        float cpuFrameTime = 0.0f;
        float gpuFrameTime = 0.0f;
        uint8_t inFlightFramesCount = 1u;
    };

    Stats gStats;

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
            // {
                const auto d = controller.getDistance();
                controller.setDistance(d - (dy / std::pow(2.0f, (8.0f - d))));

            //     const float lerpBegin = 2.0f;
            //     const float lerpEnd = 1.0f;
            //     const float lerpDisatnce = (lerpBegin - lerpEnd);
            //     const float mix = std::clamp(lerpBegin - d, 0.0f, lerpDisatnce) / lerpDisatnce;
            //     m = std::lerp(0.0f, 1.0f, mix);

            //     v0 = engine::vec3f(1.57f * m, 0.0f, 0.0f);

            //     controller.setRotation(
            //         v0 + v1
            //     );
            // }

            m = std::clamp(m + dy * 0.01f, 0.0f, 1.0f);
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
        
        // ctti example:
        static_assert(engine::ctti::typeIndex<int>() == 0u);
        static_assert(engine::ctti::typeIndex<double>() == 1u);
        static_assert(engine::ctti::UniqueTypeId<double>::typeId() == 1u);
        static_assert(engine::ctti::UniqueTypeId<int>::typeId() == 0u);
        static_assert(engine::ctti::typeIndex<int, double>() == 0u);
        static_assert(engine::ctti::typeIndex<int, int>() == 1u);
        static_assert(engine::ctti::UniqueTypeId<int>::typeId<double>() == 0u);
        static_assert(engine::ctti::UniqueTypeId<int>::typeId<int>() == 1u);

        _fileManager.createFileSystem<CommonFileSystem>("common_fs0", CommonFileSystem::getCurrentDirectory() + "./resources/");

        addInputObserver(&_imgui);
        addInputObserver(&sceneEventHandler);

        controller.setDistance(5.0f);
        controller.setMinMaxDistance(0.01f, 8.0f);

        camera.makePerspective(std::numbers::pi_v<float> / 3.0f, 1024.0f / 768.0f, 0.001f, 100.0f);
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
            _api->destroySemaphore(*_device, semaphore);
        }

        for (auto & semaphore : _transfersCompleteSemaphores) {
            _api->destroySemaphore(*_device, semaphore);
        }
        
        _commandLists.clear();
        _transferCommandLists.clear();
        _queryPool = nullptr;

        _defaultImage = nullptr;
        _sphereImage = nullptr;

        if (skeletalAnimation) {
            delete skeletalAnimation;
            skeletalAnimation = nullptr;
        }

        scene.destroy();

        _shaderModulesManager = nullptr;
        _imgui.destroy();
        debugDraw.destroy();
        
        animationManager.destroy();
        textureManager.destroy();

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
                VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
                VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME // for Resetting the query pool
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

        _swapchain = _api->createSwapchain(*_device, *_surface, swapchainCreateInfo);
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
            _depthImages.emplace_back(std::move(_api->createImage(*_device, imageInfo)));
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
            _colorAttachmentImages.emplace_back(std::move(_api->createImage(*_device, colorImageInfo)));
        }

        _sceneFrameBuffers.clear();
        _sceneFrameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};

            auto & colorImage = _colorAttachmentImages[i];
            fbAttachemnts[0u].image = colorImage.get();
            fbAttachemnts[0u].imageSize = {colorImage->getExtent().width, colorImage->getExtent().height};
            fbAttachemnts[0u].aspect = engine::rhi::ImageAspectFlags::eColor;  // can get it from colorImage?

            auto & depthImage = _depthImages[i];
            fbAttachemnts[1u].image = depthImage.get();
            fbAttachemnts[1u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
            fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?

            _sceneFrameBuffers.emplace_back(_api->createFrameBuffer(*_device, std::move(fbAttachemnts), _renderPasses[kSceneRenderPass]));
        }

        _mainFrameBuffers.clear();
        _mainFrameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            auto & image = _swapchain->getImage(i);

            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{1u};
            fbAttachemnts[0u].image = &image;
            fbAttachemnts[0u].imageSize = {image.getExtent().width, image.getExtent().height};

            _mainFrameBuffers.emplace_back(_api->createFrameBuffer(*_device, std::move(fbAttachemnts), _renderPasses[kScreenRenderPass]));
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

        sizeChanged();

        gStats.inFlightFramesCount = _swapchain->getImagesCount();
    }

    void Example::createRenderPasses() {
        const auto bytes = _fileManager.readFile("descriptors/render_pass.json");
        std::string_view input(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        engine::json::Json json{};
        const auto err = engine::json::read_json(json, input);

        const auto depthFormat = _device->getSupportedDepthFormat(32u, false);
        const auto formats0 = std::array{engine::rhi::Format::eB8G8R8A8_Unorm}; // _swapchain->getFormat()
        // const auto formats0 = std::array{engine::rhi::Format::eB8G8R8A8_Srgb}; // for my amd gpu
        const auto formats1 = std::array{engine::rhi::Format::eR8G8B8A8_Unorm, depthFormat};

        _renderPasses.emplace_back(_api->getRenderPass(*_device, engine::parseRenderPass(json["screen"], formats0))); // kScreenPass
        _renderPasses.emplace_back(_api->getRenderPass(*_device, engine::parseRenderPass(json["scene"], formats1))); // kScenePass
    }

    void Example::loadShaders() {
        _shaderModulesManager = _api->createShaderModulesManager();

        { // kImguiProgram
            auto const vBytes = _fileManager.readFile("shaders/imgui.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/imgui.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("imgui.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("imgui.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
        { // kPostEffectProgram
            auto const vBytes = _fileManager.readFile("shaders/postEffect.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/postEffect.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("postEffect.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("postEffect.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
        { // kTestProgram
            auto const vBytes = _fileManager.readFile("shaders/test.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/test.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("test.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("test.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}, {.dynamicDescriptorsMask = 0u}));
        }
        { // kComputeProgram
            auto const cBytes = _fileManager.readFile("shaders/compute_test.comp.spv");
            const auto & compModule = _shaderModulesManager->registerModule("compute_test.comp", *_device, engine::rhi::shaders::Stages::eCompute, cBytes);
            _programs.emplace_back(_api->createProgram(*_device, std::array{compModule.getView()}));
        }
        { // kDebugProgram
            auto const vBytes = _fileManager.readFile("shaders/debug.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/debug.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("debug.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("debug.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
        { // kSpritesProgram
            auto const vBytes = _fileManager.readFile("shaders/sprite.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/sprite.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("sprite.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("sprite.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kMeshProgram
            auto const vBytes = _fileManager.readFile("shaders/mesh.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/mesh.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kMeshSkeletalInstancedProgram
            auto const vBytes = _fileManager.readFile("shaders/mesh_skeletal_instanced.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/mesh.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh_skeletal_instanced.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kMeshInstancedProgram
            auto const vBytes = _fileManager.readFile("shaders/mesh_instanced.vert.spv");
            auto const fBytes = _fileManager.readFile("shaders/mesh.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh_instanced.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
    }
    
    void Example::createPipelines() {
        using namespace engine::rhi;

        loadShaders();
        _imgui.createPipeline(*_api, *_device, _programs[kImguiProgram], _renderPasses[kScreenRenderPass], 0u);
        debugDraw.createPipelines(*_api, *_device, _programs[kDebugProgram], _renderPasses[kSceneRenderPass], 0u);

        const auto bytes = _fileManager.readFile("descriptors/pipelines.json");
        std::string_view input(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        engine::json::Json json{};
        const auto err = engine::json::read_json(json, input);

        // kPostEffectPipeline
        const GraphicsPipelineStatesInfo info0 = engine::parseGraphicsPipelineInfo(json["postEffect"]);
        _pipelines.emplace_back(_api->createGraphicsPipeline(*_device, _renderPasses[kScreenRenderPass], _programs[kPostEffectProgram], info0, 0u));

        // kSceneTestPipeline
        const GraphicsPipelineStatesInfo info1 = engine::parseGraphicsPipelineInfo(json["sphere"]);
        _pipelines.emplace_back(_api->createGraphicsPipeline(*_device, _renderPasses[kSceneRenderPass], _programs[kTestProgram], info1, 0u));

        // kComputeTestPipeline
        _pipelines.emplace_back(_api->createComputePipeline(*_device, _programs[kComputeProgram]));

        // kMeshTestPipeline
        const GraphicsPipelineStatesInfo info3 = engine::parseGraphicsPipelineInfo(json["mesh"]);
        _pipelines.emplace_back(_api->createGraphicsPipeline(*_device, _renderPasses[kSceneRenderPass], _programs[kMeshProgram], info3, 0u));

        // scene pipelines
        const GraphicsPipelineStatesInfo info2 = engine::parseGraphicsPipelineInfo(json["sprite_common"]);
        scene.registerPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kSceneRenderPass], _programs[kSpritesProgram], info2, 0u));

        const GraphicsPipelineStatesInfo info4 = engine::parseGraphicsPipelineInfo(json["mesh_skeletal"]);
        scene.registerPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kSceneRenderPass], _programs[kMeshSkeletalInstancedProgram], info4, 0u));
        const GraphicsPipelineStatesInfo info5 = engine::parseGraphicsPipelineInfo(json["mesh"]);
        scene.registerPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kSceneRenderPass], _programs[kMeshInstancedProgram], info5, 0u));
    }

    engine::graphics::models::Mesh_Data meshData;

    void Example::createDefaultResources() {
        using namespace engine::rhi;
        using namespace engine::graphics;

        auto * commandPool = _device->acquireCommandPool(engine::rhi::queue_flags::transfrer);
        auto tmpCommandList = _api->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary);

        tmpCommandList->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        // gltf test
        auto const modelBytes = _fileManager.readFile("assets/models/model.glb");
        auto modelLayout = engine::graphics::models::loadModelLayout(modelBytes, "assets/models/");
        // engine::graphics::models::Mesh_Data meshData;
        meshData.loadSkins(modelLayout);
        meshData.loadAnimations(modelLayout);

        // const auto semanticsArray = std::array {engine::graphics::models::AttributeSemantic::ePosition, 
        //                                         engine::graphics::models::AttributeSemantic::eNormal | 
        //                                         engine::graphics::models::AttributeSemantic::eUv0};

        const auto semanticsArray = std::array {engine::graphics::models::AttributeSemantic::ePosition, 
                                                engine::graphics::models::AttributeSemantic::eNormal, 
                                                engine::graphics::models::AttributeSemantic::eUv0,
                                                engine::graphics::models::AttributeSemantic::eJoints,
                                                engine::graphics::models::AttributeSemantic::eWeights};

        auto  [meshVtx, meshIdx] = meshData.loadMeshes(modelLayout, semanticsArray);
        meshData.optimizeBuffers(meshVtx, meshIdx, semanticsArray);
        meshData.loadNodes(modelLayout);

        // gpu buffers
        BufferInfo meshVtxBufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        BufferInfo meshIdxBufferInfo {
            BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        _meshPositions = _api->createBuffer(*_device, meshVtxBufferInfo, meshVtx[0u].size());
        _meshNormals = _api->createBuffer(*_device, meshVtxBufferInfo, meshVtx[1u].size());
        _meshUvs = _api->createBuffer(*_device, meshVtxBufferInfo, meshVtx[2u].size());
        _meshJoints = _api->createBuffer(*_device, meshVtxBufferInfo, meshVtx[3u].size());
        _meshWeights = _api->createBuffer(*_device, meshVtxBufferInfo, meshVtx[4u].size());
        _meshIndices = _api->createBuffer(*_device, meshIdxBufferInfo, sizeof(uint32_t) * meshIdx.size());

        const auto meshVtxDataSize = meshVtx[0u].size();
        const auto meshNormalsDataSize = meshVtx[1u].size();
        const auto meshUvDataSize = meshVtx[2u].size();
        const auto meshJointsDataSize = meshVtx[3u].size();
        const auto meshWeightsDataSize = meshVtx[4u].size();
        const auto meshIdxDataSize = sizeof(uint32_t) * meshIdx.size();

        const auto stagingMeshSize = meshVtxDataSize + meshNormalsDataSize + meshUvDataSize + 
            meshJointsDataSize + meshWeightsDataSize + meshIdxDataSize;

        StagingBuffer stagingMesh{*_api, *_device, stagingMeshSize};
        std::byte * memoryMeshStaging = stagingMesh.map();
        const auto vtxOffsetMesh = stagingMesh.requestBytes(meshVtxDataSize);
        const auto normalOffsetMesh = stagingMesh.requestBytes(meshNormalsDataSize);
        const auto uvOffsetMesh = stagingMesh.requestBytes(meshUvDataSize);
        const auto jointsOffsetMesh = stagingMesh.requestBytes(meshJointsDataSize);
        const auto weightsOffsetMesh = stagingMesh.requestBytes(meshWeightsDataSize);
        const auto idxOffsetMesh = stagingMesh.requestBytes(meshIdxDataSize);

        auto * vtxDataMeshPositions = (memoryMeshStaging + vtxOffsetMesh);
        auto * vtxDataMeshNormals = (memoryMeshStaging + normalOffsetMesh);
        auto * vtxDataMeshUvs = (memoryMeshStaging + uvOffsetMesh);
        auto * vtxDataMeshJoints = (memoryMeshStaging + jointsOffsetMesh);
        auto * vtxDataMeshWeights = (memoryMeshStaging + weightsOffsetMesh);
        auto * idxDataMesh = (memoryMeshStaging + idxOffsetMesh);

        std::memcpy(vtxDataMeshPositions, meshVtx[0u].data(), meshVtxDataSize);
        std::memcpy(vtxDataMeshNormals, meshVtx[1u].data(), meshNormalsDataSize);
        std::memcpy(vtxDataMeshUvs, meshVtx[2u].data(), meshUvDataSize);
        std::memcpy(vtxDataMeshJoints, meshVtx[3u].data(), meshJointsDataSize);
        std::memcpy(vtxDataMeshWeights, meshVtx[4u].data(), meshWeightsDataSize);
        std::memcpy(idxDataMesh, meshIdx.data(), meshIdxDataSize);

        stagingMesh.flush();
        stagingMesh.unmap();

        tmpCommandList->copyBuffer2Buffer(stagingMesh.getBuffer(), _meshPositions, 
            {.srcOffset = vtxOffsetMesh, .dstOffset = 0u, .size = _meshPositions->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(stagingMesh.getBuffer(), _meshNormals, 
            {.srcOffset = normalOffsetMesh, .dstOffset = 0u, .size = _meshNormals->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(stagingMesh.getBuffer(), _meshUvs, 
            {.srcOffset = uvOffsetMesh, .dstOffset = 0u, .size = _meshUvs->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(stagingMesh.getBuffer(), _meshJoints, 
            {.srcOffset = jointsOffsetMesh, .dstOffset = 0u, .size = _meshJoints->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(stagingMesh.getBuffer(), _meshWeights, 
            {.srcOffset = weightsOffsetMesh, .dstOffset = 0u, .size = _meshWeights->sizeInBytes()});

        tmpCommandList->copyBuffer2Buffer(stagingMesh.getBuffer(), _meshIndices, 
            {.srcOffset = idxOffsetMesh, .dstOffset = 0u, .size = _meshIndices->sizeInBytes()});

        // constexpr uint32_t kMaxMemoryBytesToCopyCommand = 65536u;
        // tmpCommandList->updateBuffer(_meshPositions, 0u, meshVtx[0u].size(), meshVtx[0u].data());
        // tmpCommandList->updateBuffer(_meshIndices, 0u, meshIdx.size(), meshIdx.data());

        // gltf test

        // textureManager resources
        textureManager.createDefaultResources(*_api, *_device, tmpCommandList);

        {
            // atlas test
            auto const imageBytes = _fileManager.readFile("assets/atlases/hero-pma.png");
            auto textureData = engine::graphics::parseTextureData(imageBytes);

            using namespace engine::rhi;
            const auto imageInfo = ImageInfo{
                ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
                textureData.metaInfo.format, {textureData.metaInfo.size.x, textureData.metaInfo.size.y, 1u}
            };

            auto image = _api->createImage(*_device, imageInfo);
            const auto stagingSize = textureData.data.size();
            engine::graphics::StagingBuffer staging{*_api, *_device, stagingSize};
            
            auto * imgData = staging.map();
            std::memcpy(imgData, textureData.data.data(), stagingSize);
            staging.flush();

            tmpCommandList->copyBuffer2Image(staging.getBuffer(), image, 
                {
                    .bufferOffset = 0u,
                    .bufferRowLength = 0u,
                    .bufferImageHeight = 0u,
                    .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                    .imageOffset = {0u, 0u, 0u},
                    .imageExtent = image->getExtent()
                }
            );

            auto const atlasBytes = _fileManager.readFile("assets/atlases/hero-pma.atlas");
            auto frames = engine::graphics::FramesParser::loadFramesSpine(atlasBytes, {0u, 0u}, "hero_");

            textureManager.addFrames(frames, image);
        }

        if (1) {
            // atlas test2
            auto const imageBytes = _fileManager.readFile("assets/atlases/explosion2.png");
            auto textureData = engine::graphics::parseTextureData(imageBytes);

            using namespace engine::rhi;
            const auto imageInfo = ImageInfo{
                ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
                textureData.metaInfo.format, {textureData.metaInfo.size.x, textureData.metaInfo.size.y, 1u}
            };

            auto image = _api->createImage(*_device, imageInfo);
            const auto stagingSize = textureData.data.size();
            engine::graphics::StagingBuffer staging{*_api, *_device, stagingSize};
            
            auto * imgData = staging.map();
            std::memcpy(imgData, textureData.data.data(), stagingSize);
            staging.flush();

            tmpCommandList->copyBuffer2Image(staging.getBuffer(), image, 
                {
                    .bufferOffset = 0u,
                    .bufferRowLength = 0u,
                    .bufferImageHeight = 0u,
                    .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                    .imageOffset = {0u, 0u, 0u},
                    .imageExtent = image->getExtent()
                }
            );

            auto frames = engine::graphics::FramesParser::generateFrames({8u, 6u}, "explosion_");
            textureManager.addFrames(frames, image);

            {
                std::vector<std::string> animFrames;
                animFrames.reserve(48u);
                for (size_t i = 0u; i < 48u; ++i) {
                    animFrames.emplace_back(engine::fmt_string("explosion_%d", i));
                }

                animationManager.registerAnimation("anim0", engine::scene::FramesSequence(textureManager, animFrames));
            }
        }

        // sail test
        auto const imageBytes = _fileManager.readFile("assets/earth.ktx2");
        // auto const imageBytes = _fileManager.readFile("assets/2d_bc7.ktx2");
        auto textureData = engine::graphics::parseTextureData(imageBytes);

        const auto featureAccess24 = _device->isFormatSupported(engine::rhi::Format::eR8G8B8_Unorm, 
            engine::rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image);
        const auto featureAccess16 = _device->isFormatSupported(engine::rhi::Format::eR4G4B4A4_Unorm_Pack16, 
            engine::rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image);

        const auto featureAccessCurrent = _device->isFormatSupported(textureData.metaInfo.format, 
            engine::rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image);

        assert(
            featureAccessCurrent.checkBit(engine::rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit) ||
            featureAccessCurrent.checkBit(engine::rhi::FormatFeatureAccess::Bit::eLinearTilingFeaturesBit)
        );

        const bool sampledSupport = featureAccess24.checkBit(engine::rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit) ||
                                    featureAccess24.checkBit(engine::rhi::FormatFeatureAccess::Bit::eLinearTilingFeaturesBit);

        if (textureData.metaInfo.bpp == 24u && !sampledSupport) {
            // if (featureAccess16.optimalTilingFeatures || featureAccess16.linearTilingFeatures) {
            //     auto status = image.convert(SAIL_PIXEL_FORMAT_BPP16_RGBA);
            //     const auto bpp2 = image.bits_per_pixel();
            // } else {
                // auto status = image.convert(SAIL_PIXEL_FORMAT_BPP32_RGBA);
                // const auto bpp2 = image.bits_per_pixel();
            // }
            engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Unorm, textureData);
        }

        // create post buffers and default image
        BufferInfo bufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        _postPositions = _api->createBuffer(*_device, bufferInfo, sizeof(std::array<float, 2u>) * 4u);

        bufferInfo.usage = BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst;
        _postIndices = _api->createBuffer(*_device, bufferInfo, sizeof(uint16_t) * 6u);

        // storage buffer test
        BufferInfo storageBufferInfo {
            BufferUsageFlags::eStorage_Buffer | BufferUsageFlags::eTransfer_Src,
            MemoryPropertyFlags::eDeviceLocal
        };
        _storageBuffer = _api->createBuffer(*_device, storageBufferInfo, sizeof(std::array<float, 4u>)); // test!

        storageBufferInfo.usage = BufferUsageFlags::eTransfer_Dst;
        storageBufferInfo.memoryFlags = MemoryPropertyFlags::eHostVisible;
        _cpuStorageResult = _api->createBuffer(*_device, storageBufferInfo, sizeof(std::array<float, 4u>)); // test!
        // storage buffer test

        const auto imageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            Format::eR8G8B8A8_Unorm, {2u, 2u, 1u}
        };

        _defaultImage = _api->createImage(*_device, imageInfo);

        // staging buffer for vtx, idx, image;
        constexpr auto stagingSize = sizeof(std::array<float, 2u>) * 4u + sizeof(uint16_t) * 6u + sizeof(uint32_t) * 4u;
        StagingBuffer staging{*_api, *_device, stagingSize};
        
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
        StagingBuffer stagingSphere{*_api, *_device, stagingSphereSize};
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
        _sphereVtx = _api->createBuffer(*_device, bufferInfoSphere, sizeof(std::array<float, 5u>) * vertices.size());

        bufferInfoSphere.usage = BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst;
        _sphereIdx = _api->createBuffer(*_device, bufferInfoSphere, sizeof(uint16_t) * indices.size());
        // sphere data

        // sphere texture
        const auto sphereImageInfo = ImageInfo{
            ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
            textureData.metaInfo.format, {textureData.metaInfo.size.x, textureData.metaInfo.size.y, 1u}
        };

        _sphereImage = _api->createImage(*_device, sphereImageInfo);

        // TextureManager
        textureManager.addFrame("sphere_texture", _sphereImage);
        textureManager.addFrame("sphere_texture2", _sphereImage);

        const auto stagingSize3 = textureData.data.size();
        StagingBuffer staging3{*_api, *_device, stagingSize3};
        
        std::byte * memory3 = staging3.map();
        auto * sphereImgData = reinterpret_cast<std::byte*>(memory3);
        std::memcpy(memory3, textureData.data.data(), stagingSize3);
        staging3.flush();
        staging3.unmap();
        // sphere texture

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
        auto fence = _api->createFence(*_device, FenceCreateFlags::eNone);
        bool const submitted = _renderQueue->submit(0u, std::array{
            engine::rhi::runtime::Queue::SubmitInfo {
                .w8Semaphores = {},
                .w8DstStages = {},
                .signalSemaphores = {},
                .lists = lists
            }
        }, fence);

        _renderQueue->waitSubmit(0u, UINT64_MAX, fence);
        _api->destroyFence(*_device, fence);
        tmpCommandList = nullptr;

        // BufferInfo unifromsInfo {
        //     BufferUsageFlags::eUniform_Buffer | BufferUsageFlags::eTransfer_Dst,
        //     MemoryPropertyFlags::eDeviceLocal | MemoryPropertyFlags::eHostVisible
        // };

        BufferInfo unifromsInfo {
            BufferUsageFlags::eUniform_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        const size_t kUniformBufferOffsetAlign = _device->getUniformBufferOffsetAlignment();
        _uniformBufferI = std::make_unique<engine::graphics::InflightBuffer>(*_api, *_device, unifromsInfo, std::max(kUniformBufferOffsetAlign, sizeof(UniformBuffer)));
    
        { // update uniforms
            auto * map = _uniformBufferI->memory2<UniformBuffer>();
            map->mvp = engine::makeMatrix(1.0f);
            map->m = m;
            map->time  = 0.0f;
            _uniformBufferI->modify();
        }

        ////////////////////////////
        // auto ppp = std::shared_ptr<int>(new int{10}, [](int *a){});
        // struct D{
        //     void _(engine::rhi::runtime::sh_ptr2<int, D> *) {}
        // };

        auto const sss = sizeof(std::function<void(void * ptr)>);
        auto const sss1 = sizeof(void(*)(void * ptr));
        auto const sss2 = sizeof(std::shared_ptr<int>);
        auto const sss3 = sizeof(engine::rhi::runtime::sh_ptr<int>);
        // auto const sss4 = sizeof(engine::rhi::runtime::sh_ptr2<int, D>);

        // using NodesAllocator = allocators::PoolAllocatorChain<sizeof(engine::scene::Node), alignof(engine::scene::Node), false>;
        // NodesAllocator nodesAllocator(512u, 64u);
        // auto * rootNode = nodesAllocator.create<engine::scene::Node>();
        // auto * node0 = nodesAllocator.create<engine::scene::Node>();
        // auto * node1 = nodesAllocator.create<engine::scene::Node>();

        // rootNode->addChild(node0);
        // node0->addChild(node1);

        // engine::mat4f m0{1.0F};
        // m0[3] = {10.0f, 0.0f, 0.0f, 1.0f};

        // engine::mat4f m1{1.0F};
        // m1[3] = {20.0f, 0.0f, 0.0f, 1.0f};

        // node0->setTransform(m0);
        // node1->setTransform(m1);

        // engine::scene::recalculateHierarchy(rootNode, false, [](engine::scene::Node*) noexcept { return true; });

        // if (node0->removeChild(node1)) {
        //     nodesAllocator.destroy(node1);
        // }
        // auto * node2 = nodesAllocator.create<engine::scene::Node>();
        // node0->addChild(node2);

        // engine::scene::recalculateHierarchy(rootNode, false, [](engine::scene::Node*) noexcept { return true; });

        // int a = 0;

        // scene
        if (true) {
            sceneWorldRoot = scene.createNode();
            scene.rootNode()->addChild(sceneWorldRoot);

            for (size_t i = 0; i < 10; ++i) { // mesh hierarchy test
                vec3f minCorner{std::numeric_limits<float>::max()};
                vec3f maxCorner{std::numeric_limits<float>::lowest()};

                auto loadNode = [&](const auto & self, const uint16_t nodeId, engine::scene::Node * parent, engine::scene::Skeleton * skeleton) -> engine::scene::Node* {
                    auto * current = scene.createNode();
                    if (parent) {
                        parent->addChild(current);
                    }

                    if (skeleton) {
                        skeleton->node(nodeId) = current;
                    }

                    const auto & node = meshData.nodes[nodeId];
                    current->setScale(vec3f{node.scale.x, node.scale.y, node.scale.z});
                    current->setRotation(quatf{node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z});
                    current->setPosition(vec3f{node.translation.x, node.translation.y, node.translation.z});

                    current->applyTransformations();
                    current->calculateWorldTransform(true);

                    if (node.mesh != engine::graphics::models::kInvalidValue) {
                        bool const createSkeletalMesh = skeleton != nullptr && node.skin != engine::graphics::models::kInvalidValue;
                        const auto pipelineId = createSkeletalMesh ? 1u : 2u;
                        auto * mesh = scene.createObject<engine::scene::Mesh>(meshData, node.mesh, node.skin, pipelineId);
                        
                        mesh->setIndices({_meshIndices});
                        mesh->setAttribute({_meshPositions}, engine::scene::mesh::Semantic::ePosition);
                        mesh->setAttribute({_meshNormals}, engine::scene::mesh::Semantic::eNormal);
                        mesh->setAttribute({_meshUvs}, engine::scene::mesh::Semantic::eUv0);
                        
                        if (createSkeletalMesh) {
                            mesh->setSkeleton(skeleton);
                            mesh->setAttribute({_meshJoints}, engine::scene::mesh::Semantic::eJoints);
                            mesh->setAttribute({_meshWeights}, engine::scene::mesh::Semantic::eWeights);
                        }

                        engine::scene::attachObjectToNode(*mesh, *current);

                        for (const auto & p : meshData.meshes[node.mesh].primitives) {
                            const vec3f corner1 = current->worldTransform * vec4f(p.minCorner, 1.0f);
				            const vec3f corner2 = current->worldTransform * vec4f(p.maxCorner, 1.0f);

                            minCorner.x = std::min({minCorner.x, corner1.x, corner2.x});
                            minCorner.y = std::min({minCorner.y, corner1.y, corner2.y});
                            minCorner.z = std::min({minCorner.z, corner1.z, corner2.z});

                            maxCorner.x = std::max({maxCorner.x, corner1.x, corner2.x});
                            maxCorner.y = std::max({maxCorner.y, corner1.y, corner2.y});
                            maxCorner.z = std::max({maxCorner.z, corner1.z, corner2.z});
                        }
                    }

                    for (const uint16_t cNodeId : node.children) {
                        self(self, cNodeId, current, skeleton);
                    }
                    return current;
                };

                auto * meshRoot = scene.createNode();
                engine::scene::Skeleton * skeleton = scene.createObject<engine::scene::Skeleton>(meshData);

                for (const uint16_t nodeId : meshData.sceneNodes) {
				    auto * node = loadNode(loadNode, nodeId, meshRoot, skeleton);
			    }

                // engine::scene::hierarchyPass(meshRoot, [](engine::scene::Node * n) noexcept{ });
                // engine::scene::recalculateHierarchy(meshRoot, [](engine::scene::Node * n) noexcept { 
                //     return true;
                // });
                if (skeleton) {
                    engine::scene::attachObjectToNode(*skeleton, *meshRoot);
                }

                auto const center = (minCorner + maxCorner) * 0.5f;
                auto const radius = std::max({
                    std::abs(minCorner.x - center.x), std::abs(minCorner.y - center.y), std::abs(minCorner.z - center.z),
                    std::abs(maxCorner.x - center.x), std::abs(maxCorner.y - center.y), std::abs(maxCorner.z - center.z)
                });
                meshRoot->setBounds(engine::scene::Bounds::createSphere(center, radius));
                // meshRoot->setBounds(engine::scene::Bounds::createCube(minCorner, maxCorner));

                meshRoot->setScale(engine::common::random(0.9f, 1.1f));
                meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, engine::common::random(-3.14f, 3.14f)));
                meshRoot->setPosition(engine::vec3f(-4.0f + i * 1.25f, 0.0f, 0.0f));
                // meshRoot->setScale(0.002f);
                // meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, 0.0f));
                // meshRoot->setPosition(engine::vec3f(0.0f, 0.0f, 0.0f));

                if (i == 5u) {
                    tmpSkeleton = skeleton;
                    skeletalAnimation = new engine::scene::SkeletalAnimation(meshData.animations[0u]);
                    gSkeletalAnimator.enlarge2(skeleton->nodesCount());
                }
                
                sceneWorldRoot->addChild(meshRoot);
            }
            
            auto m0 = engine::makeMatrix(0.5F);
            engine::rotateMatrix_z(m0, 1.57f);
            m0[3] = {1.0f, 0.0f, 0.0f, 1.0f};

            auto m1 = engine::makeMatrix(0.75F);
            m1[3] = {1.5f, 0.0f, 0.0f, 1.0f};

            auto * node0 = scene.createNode();
            node0->setScale(0.5f);
            node0->setRotation(engine::vec3f{0.0f, 0.0f, 1.57f});
            node0->setPosition(engine::vec3f{1.0f, 0.0f, 0.0f});
            // node0->setTransform(m0);
            node0->setBounds(engine::scene::Bounds::createSphere(vec3f{0.0f}, 0.5f));
            sceneWorldRoot->addChild(node0);

            auto * node1 = scene.createNode();
            node1->setTransform(m1);
            node1->setBounds(engine::scene::Bounds::createSphere(vec3f{0.0f}, 0.5f));
            node0->addChild(node1);

            engine::rotateMatrix_y(m0, -0.76f);
            m0[3] = {-1.0f, 2.0f, 0.0f, 1.0f};
            auto * node2 = scene.createNode();
            node2->setTransform(m0);
            node2->setBounds(engine::scene::Bounds::createCube(vec3f{-0.75f}, vec3f{0.75f}));
            sceneWorldRoot->addChild(node2);

            auto * object0 = scene.createObject<engine::scene::Dummy>();
            auto * object1 = scene.createObject<engine::scene::Dummy>();

            auto * finalNode0 = scene.createNode();
            auto * finalNode1 = scene.createNode();
            engine::scene::attachObjectToNode(*object0, *finalNode0);
            engine::scene::attachObjectToNode(*object1, *finalNode1);

            node1->addChild(finalNode0);
            node2->addChild(finalNode1);

            object0->setOrder(1u);
            object1->setOrder(1u);
            // node2->setHidden(true);
            // object1->node->setHidden(true);

            // if (scene.destroyObject(object1)) {
                // scene.destroyNode(node2);
            // }

            // engine::scene::Transformation transform(0.01f);
            // transform.setRotation(engine::vec3f{1.0f, 0.0f, 0.0f});
            // transform.setPosition(engine::vec3f(0.0f, 2.0f, 0.0f));
            // auto mt = engine::mat4f{};
            // transform.apply(mt);

            auto * node3 = scene.createNode();
            node3->setScale(engine::vec3f{0.01f});
            node3->setRotation(engine::vec3f{1.0f, 0.0f, 0.0f});
            node3->setPosition(engine::vec3f(0.0f, 2.0f, 0.0f));
            // node3->setTransform(mt);

            auto m2 = engine::makeMatrix(0.01F);
            auto * sprite = scene.createObject<engine::scene::Sprite>(*textureManager.findFrame("hero_head"), 0u);
            engine::scene::attachObjectToNode(*sprite, *node3);

            m2[1][1] *= -1.0f;
            m2[3] = {0.0f, -0.2f, 0.5f, 1.0f};
            auto * node4 = scene.createNode();
            node4->setRotation(engine::vec3f{1.57f, 0.2f, 3.0f});
            node4->setScale(0.015f);
            node4->setPosition(engine::vec3f(0.57f, 0.05f, -0.85f));
            //node4->setTransform(m2);
            
            auto * sprite2 = scene.createObject<engine::scene::Sprite>(*textureManager.findFrame("hero_sword"), 0u);
            engine::scene::attachObjectToNode(*sprite2, *node4);
            sprite2->setOrder(1u);

            sceneWorldRoot->addChild(node3);

            auto * handBone = tmpSkeleton ? tmpSkeleton->node(15u) : nullptr;
            if (handBone) {
                handBone->addChild(node4);
            } else {
                sceneWorldRoot->addChild(node4);
            }

            {
                m2[0][0] *= 0.25f;
                m2[1][1] *= -0.25f;

                auto const animId = animationManager.getAnimationIndex("anim0");

                for (size_t i = 0u; i < 10u; ++i) {
                    for (size_t j = 0u; j < 10u; ++j) {
                        auto * node5 = scene.createNode();
                        node5->setScale(engine::vec3f{0.0025f});
                        node5->setPosition(engine::vec3f{-3.0f + static_cast<float>(i) * 0.75f, -1.0f - static_cast<float>(j) * 0.75f, 0.0f});

                        const auto * frame3 = textureManager.findFrame("explosion_33");
                        auto * sprite3 = scene.createObject<engine::scene::Sprite>(*frame3, 0u);
                        engine::scene::attachObjectToNode(*sprite3, *node5);
                        sprite3->setOrder(-1);
                        auto * anim0 = scene.createAnimInfo<engine::scene::FramesSequence>(animId, engine::common::random(0.25f, 1.7f));
                        scene.destroyAnimInfo(sprite3->setAnimation(anim0));
                        
                        sceneWorldRoot->addChild(node5);
                    }
                }
            }
            
            sceneNodePtr = node0;
            sizeof(engine::scene::Bounds);
            sizeof(engine::scene::Node);
            sizeof(engine::scene::Transformation);
        }

    }

    void Example::prepare() {
        const auto inflightFrames = _device->inFlightFramesCount();
        _renderQueue = _api->getQueue(*_device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, inflightFrames);

        _readyToPresentSemaphores.resize(inflightFrames);
        for (auto & semaphore : _readyToPresentSemaphores) {
            semaphore = _api->createSemaphore(*_device);
        }

        _transfersCompleteSemaphores.resize(inflightFrames);
        for (auto & semaphore : _transfersCompleteSemaphores) {
            semaphore = _api->createSemaphore(*_device);
        }

        _frameIndex.setFramesCount(inflightFrames);

        _imgui.createFontTexture(*_api, *_device);
        _imgui.createBuffers(*_api, *_device);
        debugDraw.createBuffers(*_api, *_device);
        scene.initRenderData(*_api, *_device);

        // on render thread
        auto * commandPool = _device->acquireCommandPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
        _commandLists.reserve(inflightFrames);
        for (size_t i = 0u; i < inflightFrames; ++i) {
            _commandLists.emplace_back(_api->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary));
        }

        _transferCommandLists.reserve(inflightFrames);
        for (size_t i = 0u; i < inflightFrames; ++i) {
            _transferCommandLists.emplace_back(_api->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary));
        }

        // queryPool
        _queryPool = _api->createQueryPool(*_device, {
            .queryType = engine::rhi::QueryType::eTimestamp,
            .queryCount = inflightFrames * 2u
        });

        // _queryResults[0] contains the queried timestamp
        // _queryResults[1] contains availability of the timestamp
        // and we get 2 timestamps
        _queryResults.resize(inflightFrames);
        for (auto & result : _queryResults) {
            result.resize(4u); // need 4 values for get 2 timestamps wits rhi::QueryResultFlags::eWith_Availability flag!
        }

        //
        createDefaultResources();
    }

    void Example::deviceProcess() {
        if (controller.calculateTransform() && _renderThread) {
            _renderThread->emplaceTasks(stl::make_vector(threads::WorkerTask{[transform = camera.getTransform()](){
                cameraTransform = transform;
            }}));
        }
    }

    void Example::update(threads::ExecutionTimePoint now, float delta) {

        if (_mainThread) {
            // _mainThread->emplaceTasks(stl::make_vector(threads::WorkerTask{[](){
            //     LOG_D("test main thread task");
            // }}));
        }

        const auto [width, height] = _swapchain->getSize();
        _imgui.beginFrame(delta, width, height);
        // ImGui::ShowDemoWindow();
        // ImGui::ShowMetricsWindow();

        static bool showDemo = false;
        if (showDemo) {
            ImGui::ShowDemoWindow();
        }
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("#\t")) {
                if (ImGui::MenuItem("Info")) {
                }
                if (ImGui::MenuItem("Demo")) {
                    showDemo = !showDemo;
                }
                ImGui::EndMenu();
            }

            ImGui::Text("frames inflight: %d\t", gStats.inFlightFramesCount);
            auto str = engine::fmt_string("fps: %.1f\t", 1'000.0f / gStats.cpuFrameTime);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }
            str = engine::fmt_string("cpu time: %.3f ms\t", gStats.cpuFrameTime);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }
            str = engine::fmt_string("gpu time: %.3f ms\t", gStats.gpuFrameTime);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }
            
            ImGui::EndMainMenuBar();
        }

        _imgui.endFrame();

        // debugDraw.drawLines(std::array{engine::vec3f{-2.0f, 0.0f, 0.0f}, engine::vec3f{2.0f, 0.0f, 0.0f}}, 0xff0000ffu, cameraTransform);
        // debugDraw.drawLines(std::array{engine::vec3f{0.0f, -2.0f, 0.0f}, engine::vec3f{0.0f, 2.0f, 0.0f}}, 0xff00ff00u, cameraTransform);
        // debugDraw.drawLines(std::array{engine::vec3f{0.0f, 0.0f, -2.0f}, engine::vec3f{0.0f, 0.0f, 2.0f}}, 0xffff0000u, cameraTransform);

        // debugDraw.drawLines(std::array{engine::vec3f{-0.5f, -0.4f, 0.0f}, engine::vec3f{0.5f, -0.4f, 0.0f}}, 0xff000000u);
        // // debugDraw.drawLines(std::array{engine::vec3f{-0.25f, -0.2f, 0.0f}, engine::vec3f{0.25f, -0.2f, 0.0f}, engine::vec3f{0.25f, -0.75f, 0.0f}, engine::vec3f{-0.25f, -0.75f, 0.0f}}, 0xff00ff00u);
        // debugDraw.drawSquare(engine::vec2f{-0.25f, -0.2f}, engine::vec2f{0.25f, -0.9f}, 0x5500ff00u, {1.0f}, engine::debug::DebugDraw::Type::eLines);
        // debugDraw.drawSquare(engine::vec2f{-0.1f, -0.3f}, engine::vec2f{0.1f, -0.5f}, 0x55ff0000u, {1.0f}, engine::debug::DebugDraw::Type::eFill);
        // debugDraw.drawSquare(engine::vec2f{-1.1f, -1.6f}, engine::vec2f{1.1f, -1.8f}, 0x55ff0000u, cameraTransform, engine::debug::DebugDraw::Type::eFull);

        // debugDraw.drawCircle(engine::vec2f{0.0f, 0.0f}, 2.2f, 0x22ffffffu, 16u, cameraTransform, engine::debug::DebugDraw::Type::eFull);
        // debugDraw.drawSphere(engine::vec3f{1.0f, 1.0f, 1.2f}, 0.4f, 0x2222ffffu, 32u, cameraTransform, engine::debug::DebugDraw::Type::eFull);

        // debugDraw.drawCube(engine::vec3f{1.5f, -0.5f, -0.5f}, engine::vec3f{2.5f, 0.5f, 0.5f}, 0x22ff22ffu, cameraTransform, engine::debug::DebugDraw::Type::eFull);
    
        if (sceneNodePtr) {
            static bool n = false;
            auto position = sceneNodePtr->getPosition();
            if (!n) {
                if (position.z < 1.0f) {
                    position.z += 1.0f * delta;
                } else {
                    n = true;
                }
            } else {
                if (position.z > 0.0f) {
                    position.z -= 1.0f * delta;
                } else {
                    n = false;
                }
            }
            sceneNodePtr->setPosition(position);
            // sceneNodePtr->setTransform(tr);
        }

        if (skeletalAnimation && tmpSkeleton) {
            const std::chrono::duration<float> nowDuration = now.time_since_epoch();
            const float nowTimeCount = nowDuration.count();

            gSkeletalAnimator.update2(nowTimeCount, *skeletalAnimation);
            gSkeletalAnimator.apply2(*tmpSkeleton);
        }

        std::vector<engine::scene::AttachmentObject*> objects(1024u);
        scene.fillAttachmentsArray(cameraTransform, sceneWorldRoot, [](engine::scene::AttachmentObject*) { return true; }, objects);
        scene.update(now, delta, objects, animationManager);
    }

    void Example::prepareFrameDrawData(engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, float delta) {
        using namespace engine;
        { // update uniforms
            auto * map = _uniformBufferI->memory2<UniformBuffer>();
            //rotateMatrix_z(*map, -delta);

            map->mvp = cameraTransform;
            map->m = m;
            map->time += delta * 0.01f;

            _uniformBufferI->modify();
            _uniformBufferI->commit(*commandList, frame);
        }

        if constexpr (!kUseTransferCommandListForPrepareImgui) {
            _imgui.prepare(*commandList, frame, *_api, *_device);
        }
    }

    void Example::draw(threads::ExecutionTimePoint now, float delta) {
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
        auto & transferCommandList = _transferCommandLists[frameId];
        auto & commandList = _commandLists[frameId];
        transferCommandList->begin(rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        commandList->begin(rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        commandList->resetQueryPool(_queryPool.get(), frameId * 2u, 2u);
        commandList->writeTimestamp(_queryPool.get(), rhi::PipelineStageFlags::eTop_Of_Pipe, frameId * 2u);

        // prepare frame draw data (update uniforms and copy buffers memory)
        prepareFrameDrawData(commandList, frameId, delta);

        // compute
        static uint8_t computed = 0u;
        if (computed == 0u) {
            commandList->bindPipeline(_pipelines[kComputeTestPipeline]);
            commandList->bindBuffer(0u, _storageBuffer);
            commandList->dispatch(1u, 1u, 1u);

            commandList->copyBuffer2Buffer(_storageBuffer, _cpuStorageResult, {
                .srcOffset = 0u,
                .dstOffset = 0u, 
                .size = sizeof(std::array<float, 4u>)
            });
            computed = 1u;
        } else if (computed == 1u && frameId == 0u) { // can get result, compute gpu job finished!
            const auto v4 = *_cpuStorageResult->map2<vec4f>();
            LOG_I("compute shader result: {%.2f, %.2f, %.2f, %.2f}", v4.x, v4.y, v4.z, v4.w);
            _cpuStorageResult->unmap();
            computed = 2u;
        }

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

        // // sphere
        // commandList->bindPipeline(_pipelines[kSceneTestPipeline]);
        // commandList->bindVertexBuffer(0u, _sphereVtx);
        // commandList->bindIndexBuffer(_sphereIdx, 0u, rhi::IndexType::eU16);

        // // const auto * frame = textureManager.findFrame("sphere_texture");
        // // commandList->bindImage(1u, frame->getImage()/*_sphereImage*/, rhi::SamplerValue {
        // //     .info = {
        // //         .magFilter = rhi::FilterMode::eLinear,
        // //         .minFilter = rhi::FilterMode::eLinear
        // //     }
        // // });

        // // const auto * frame = textureManager.findFrame("upper-arm1");
        // // commandList->bindImage(1u, frame->getImage(), rhi::SamplerValue {
        // //     .info = {
        // //         .magFilter = rhi::FilterMode::eLinear,
        // //         .minFilter = rhi::FilterMode::eLinear
        // //     }
        // // });

        // commandList->bindImage(1u, textureManager.getDefaultImage(), rhi::SamplerValue {
        //     .info = {
        //         .magFilter = rhi::FilterMode::eNearest,
        //         .minFilter = rhi::FilterMode::eNearest
        //     }
        // });

        // commandList->bindBuffer(0u, (*_uniformBufferI)(frameId), 0u, 0u, sizeof(UniformBuffer));

        // // for (size_t i = 0; i < 2'000; ++i) {
        //     commandList->drawIndexed(_sphereIdx->sizeInBytes() / sizeof(uint16_t), 1u);
        // // }
        // // sphere

        // mesh test
        commandList->bindPipeline(_pipelines[kMeshTestPipeline]);
        commandList->bindVertexBuffer(0u, _meshPositions);
        commandList->bindVertexBuffer(1u, _meshNormals);
        commandList->bindVertexBuffer(2u, _meshUvs);
        commandList->bindIndexBuffer(_meshIndices, 0u, rhi::IndexType::eU32);
        commandList->bindBuffer(0u, (*_uniformBufferI)(frameId), 0u, 0u, sizeof(UniformBuffer));
        // commandList->drawIndexed(_meshIndices->sizeInBytes() / sizeof(uint32_t), 4'000u);
        // commandList->drawIndexed(31890u, 4'000u, 0u);
        // commandList->drawIndexed(1017u, 4'000u, 31890u);
        // mesh test

        scene.newFrame(now);
        std::vector<engine::scene::AttachmentObject*> objects(1024u);
        scene.fillAttachmentsArray(cameraTransform, sceneWorldRoot, [](engine::scene::AttachmentObject*) { return true; }, objects);
        scene.draw(commandList, transferCommandList, frameId, cameraTransform, objects);

        debugDraw.prepare(*transferCommandList, frameId);
        debugDraw.draw(*commandList, frameId, now);

        //commandList->nextSubpass(rhi::SubpassContents::ePrimaryBufferInline);
        commandList->endRenderPass();

        // final pass
        std::array images2Shaders{rhi::runtime::ImageTransitionInfo{_colorAttachmentImages[imageIndex].get()}};
        commandList->beginRenderPass(_renderPasses[kScreenRenderPass], _mainFrameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}}, images2Shaders);

        // draw with postEffect
        commandList->bindPipeline(_pipelines[kPostEffectPipeline]);
        commandList->bindVertexBuffer(0u, _postPositions);
        commandList->bindIndexBuffer(_postIndices, 0u, rhi::IndexType::eU16);
        commandList->bindImage(0u, _colorAttachmentImages[imageIndex], rhi::SamplerValue{});
        commandList->drawIndexed(6u);

        // draw imgui
        if constexpr (kUseTransferCommandListForPrepareImgui) {
            _imgui.prepare(*transferCommandList, frameId, *_api, *_device);
        }
        _imgui.draw(*commandList, frameId);

        commandList->endRenderPass();

        commandList->writeTimestamp(_queryPool.get(), rhi::PipelineStageFlags::eBottom_Of_Pipe, frameId * 2u + 1u);
        commandList->end();
        transferCommandList->end();

        transferCommandList->record(frameId);
        commandList->record(frameId);
        // render frame

        // query pool results
        auto results = std::span{reinterpret_cast<std::byte*>(_queryResults[frameId].data()), 
            _queryResults[frameId].size() * sizeof(uint64_t)};
        _queryPool->getResults(frameId * 2u, 2u, results, 2 * sizeof(uint64_t), 
            rhi::QueryResultFlags::e64 | rhi::QueryResultFlags::eWith_Availability);

        if (_queryResults[frameId][3] != 0) {
            const float gpuFrameTime = _gpu->timestampToMilliseconds(_queryResults[frameId][2] - _queryResults[frameId][0]);
            gStats.cpuFrameTime = delta * 1'000.0f;
            gStats.gpuFrameTime = gpuFrameTime;
        }

        // submit
        // const auto waitSemaphores = std::array{_swapchain->imageAvailableSemaphore(frameId)};
        // const auto signalSemaphores = std::array{_readyToPresentSemaphores[frameId]};
        // const auto waitStages = std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output};
        // auto lists = std::array{commandList.get()};

        // bool const submitted = _renderQueue->submit(frameId, std::array{
        //     engine::rhi::runtime::Queue::SubmitInfo {
        //         .w8Semaphores = waitSemaphores,
        //         .w8DstStages = waitStages,
        //         .signalSemaphores = signalSemaphores,
        //         .lists = lists
        //     }
        // });

        // draw after transfer variant
        // const auto transferWaitSemaphores = std::array{_swapchain->imageAvailableSemaphore(frameId)};
        // const auto transferSignalSemaphores = std::array{_transfersCompleteSemaphores[frameId]};
        // const auto transferWaitStages = std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output};
        // auto transferLists = std::array{transferCommandList.get()};

        // const auto waitSemaphores = transferSignalSemaphores;
        // const auto signalSemaphores = std::array{_readyToPresentSemaphores[frameId]};
        // const auto waitStages = std::array{engine::rhi::PipelineStageFlags::eTransfer};
        // auto lists = std::array{commandList.get()};

        // bool const submitted = _renderQueue->submit(frameId, std::array{
        //     engine::rhi::runtime::Queue::SubmitInfo {
        //         .w8Semaphores = transferWaitSemaphores,
        //         .w8DstStages = transferWaitStages,
        //         .signalSemaphores = transferSignalSemaphores,
        //         .lists = transferLists
        //     },
        //     engine::rhi::runtime::Queue::SubmitInfo {
        //         .w8Semaphores = waitSemaphores,
        //         .w8DstStages = waitStages,
        //         .signalSemaphores = signalSemaphores,
        //         .lists = lists
        //     }
        // });

        const auto waitSemaphores = std::array{_swapchain->imageAvailableSemaphore(frameId)};
        const auto signalSemaphores = std::array{_readyToPresentSemaphores[frameId]};
        const auto waitStages = std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output};

        auto transferLists = std::array{transferCommandList.get()};
        auto lists = std::array{commandList.get()};

        bool const submitted = _renderQueue->submit(frameId, std::array{
            engine::rhi::runtime::Queue::SubmitInfo {
                .w8Semaphores = {},
                .w8DstStages = {},
                .signalSemaphores = {},
                .lists = transferLists
            },
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