#include "example.h"

#include "threads/executor.h"
#include "threads/worker.h"
#include "file/common_file_system.h"
#include "common/random.h"
#include "graphics/render/rhi/vulkan/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include "graphics/render/staging_buffer.h"

#include "math/math.h"
#include "math/geometry.h"

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

#include "graphics/common/graphics_service.h"
#include "graphics/common/draw_helper.h"
#include "graphics/texture/atlas.h"
#include "graphics/scene/animation/texture/frame_sequence_animation.h"
#include "graphics/scene/animation/mesh/skeletal_animation.h"
#include "graphics/scene/animation_manager.h"
#include "graphics/scene/scene_helper.h"
#include "graphics/mesh/model_manager.h"
#include "graphics/common/material.h"
#include "graphics/common/background_command_context.h"
#include "graphics/render/techniques/cascade_shadow_map.h"
#include "graphics/render/techniques/ssao.h"

#include "network/network_service_curl.h"

#include "utils/stl_helper.h"
#include <vtzero/vector_tile.hpp>
#include <vtzero/property_value.hpp>

#include <deque>
#include <optional>

// https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
// https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
// https://nikitablack.github.io/post/how_to_use_vulkan_timestamp_queries/

namespace example {
    constexpr uint8_t kDepthFormatTargetBits = 32u;

    constexpr uint8_t kScreenRenderPass = 0u;
    constexpr uint8_t kSceneRenderPass = 1u;
    constexpr uint8_t kGbufferRenderPass = 2u;
    constexpr uint8_t kBlurRenderPass = 3u;

    constexpr uint8_t kImguiProgram = 0u;
    constexpr uint8_t kPostEffectProgram = 1u;
    constexpr uint8_t kDebugProgram = 2u;
    constexpr uint8_t kSpritesProgram = 3u;
    constexpr uint8_t kMeshProgram = 4u;
    // constexpr uint8_t kMeshSkeletalInstancedProgram = 5u;
    constexpr uint8_t kMeshInstancedProgram = 5u;
    constexpr uint8_t kComputeProgram = 6u;
    constexpr uint8_t kMeshAfterComputeProgram = 7u;
    constexpr uint8_t kSsaoProgram = 8u;
    constexpr uint8_t kEmissiveBlurProgram = 9u;
    constexpr uint8_t kMeshShadowProgram = 10u;
    constexpr uint8_t kMeshAfterComputeShadowProgram = 11u;

    constexpr uint8_t kMeshInstancedProgramNoPbr = 12u;
    constexpr uint8_t kMeshAfterComputeProgramNoPbr = 13u;

    constexpr uint8_t kPostEffectPipeline = 0u;
    constexpr uint8_t kSsaoPipeline = 1u;
    constexpr uint8_t kEmissiveBlurPipeline = 2u;

    constexpr uint8_t kSpritesProgram2 = 14u;

    constexpr uint8_t kSsaoKernelSize = 32u;
    constexpr uint8_t kSsaoNoiseDim = 4u;

    engine::graphics::DrawHelper drawHelper;
    
    engine::FileManager fileManager;
    engine::graphics::MaterialManager materialManager;
    engine::scene::AnimationManager animationManager;
    std::optional<engine::graphics::ModelManager> modelManager;
    std::optional<engine::graphics::TextureManager> textureManager;
    std::optional<engine::graphics::GraphicsService> graphicsService{};
    std::optional<engine::scene::Scene> scene{};

    // engine::vec2f kNearFarPlane = {0.001f, 100.0f}; // common depth
    engine::vec2f kNearFarPlane = {100.0f, 0.01f}; // reverse depth
    engine::scene::Camera camera;
    engine::scene::SphereCameraController controller{camera};
    engine::scene::Node * sceneWorldRoot = nullptr;
    engine::scene::Node * swordNode = nullptr;

    std::vector<engine::scene::AttachmentObject*> shadowObjects(1024u);
    std::vector<engine::scene::AttachmentObject*> objects(1024u);

    std::unique_ptr<engine::graphics::CascadeShadowMap> shadowMap = nullptr;
    std::unique_ptr<engine::graphics::InflightBuffer> shadowMapUniformBuffer = nullptr;
    struct ShadowMapUniforms {
        std::array<engine::mat4f, 2u> cascades;
        engine::vec4f splits;
        engine::vec4f options;
    };

    engine::graphics::Ssao ssao;

    //
    engine::rhi::runtime::ImagePtr skyBoxCubeMapImage = nullptr;

    // options
    bool showOptions = false;
    
    enum class DebugShow : uint8_t {
        eCommon = 0u,
        eSsao = 1u,
        eAlbedo = 2u,
        eEmissive = 3u,
        eMetallicRoughness = 4u,
        eWorldPos = 5u,
        eNormal = 6u,
        eShadowCascade0 = 7u,
        eShadowCascade1 = 8u,
        eDebugCascades = 9u
    };

    float sceneAnimationSpeed = 1.0f;
    float ssaoEnable = 1.0f;
    bool ssaoBlur = true;
    bool showBboxes = false;
    DebugShow debugShow = DebugShow::eCommon;
    float ssaoRadius = 0.07f;
    float ssaoBias = 0.001f;
    float ssaoRangeDepth = 5.0f;
    enum class SsaoRangeMethod : uint8_t {
        eSmoothstep = 0u,
        eClamp = 1u
    };
    SsaoRangeMethod ssaoRangeMethod = SsaoRangeMethod::eSmoothstep;

    bool emissiveBlur = true;
    float emissiveBlurScale = 2.2f;
    float emissiveBlurStrength = 1.6f;
    int emissiveBlurSteps = 2;

    float vignetteExtent = 0.1f;

    uint32_t clearColor = 0xcd'cd'cd'ffu;
    engine::vec4f ambientColor = {1.0f, 1.0f, 1.0f, 0.2f};
    engine::vec4f directionalLightColor = {1.0f, 1.0f, 1.0f, 4.200f};
    engine::vec3f lightDir = {0.26f, 0.15f, -0.75f};
    engine::vec4f shadowsOptions = {0.2f, 0.0f, 0.0f, 0.0f};
    bool lightDirty = false;
    float saturation = 1.0f;

    engine::network::CurlNetworkService curlNetworkService;

    void optionsWindow() {
        ImGui::SetNextWindowSize(ImVec2(540, 840), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("options", &showOptions, 0)) {
            // early out if the window is collapsed, as an optimization.
            ImGui::End();
            return;
        }

        static float color[3u] = {
            static_cast<float>(0xffu & (clearColor >> 24u)) / 255.0f, 
            static_cast<float>(0xffu & (clearColor >> 16u)) / 255.0f,
            static_cast<float>(0xffu & (clearColor >> 8u)) / 255.0f
        };
        if (ImGui::ColorEdit3("clear color", &color[0u])) {
            clearColor = static_cast<uint32_t>(color[0u] * 255u) << 24u | 
                         static_cast<uint32_t>(color[1u] * 255u) << 16u |
                         static_cast<uint32_t>(color[2u] * 255u) << 8u | 
                         255u;
        }
        ImGui::NewLine();

        bool enableSsao = ssaoEnable > 0.0f;
        ImGui::Checkbox("ssao", &enableSsao);
        ImGui::SameLine();
        ImGui::Checkbox("blur", &ssaoBlur);
        ImGui::SameLine();
        ImGui::Checkbox("bboxes", &showBboxes);
        ssaoEnable = enableSsao ? (ssaoBlur ? 3.0f : 1.0f) : 0.0f;
        ImGui::DragFloat("ssao radius", &ssaoRadius, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("ssao bias", &ssaoBias, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("ssao range depth", &ssaoRangeDepth, 0.05f, 0.0f, 100.0f);
        ImGui::Text("range method:");
        static int ssaoRange = 0;
        if (ImGui::RadioButton("smoothstep", ssaoRangeMethod == SsaoRangeMethod::eSmoothstep)) { 
            ssaoRangeMethod = SsaoRangeMethod::eSmoothstep; 
         }
        if (ImGui::RadioButton("clamp", ssaoRangeMethod == SsaoRangeMethod::eClamp)) { 
            ssaoRangeMethod = SsaoRangeMethod::eClamp;
        }
        
        ImGui::NewLine();
        ImGui::Checkbox("blur emissive", &emissiveBlur);
        ImGui::DragInt("blur steps", &emissiveBlurSteps, 1, 0, 8);
        ImGui::DragFloat("blur scale", &emissiveBlurScale, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("blur strength", &emissiveBlurStrength, 0.1f, 0.0f, 10.0f);
        
        ImGui::NewLine();
        if (ImGui::DragFloat3("light direction", &lightDir[0u], 0.005f, -1.0f, 1.0f)) {
            lightDir.z = std::min(lightDir.z, -0.01f);
            lightDirty = true;
        }
        
        ImGui::ColorEdit3("ambient color", &ambientColor[0u]);
        ImGui::DragFloat("ambient factor", &ambientColor[3u], 0.01f, 0.0f, 1.0f);
        ImGui::ColorEdit3("directional light color", &directionalLightColor[0u]);
        ImGui::DragFloat("directional light factor", &directionalLightColor[3u], 0.01f, 0.0f, 10.0f);
        if (ImGui::DragFloat("shadow factor", &shadowsOptions[0u], 0.01f, 0.0f, 1.0f)) {
            lightDirty = true;
        }
        
        ImGui::NewLine();
        ImGui::DragFloat("saturation", &saturation, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("vignette", &vignetteExtent, 0.01f, 0.0f, 1.0f);

        ImGui::NewLine();
        ImGui::DragFloat("animation speed", &sceneAnimationSpeed, 0.01f, -4.0f, 4.0f);

        ImGui::NewLine();
        if (ImGui::RadioButton("composite", debugShow == DebugShow::eCommon)) { 
            debugShow = DebugShow::eCommon;
         }
        if (ImGui::RadioButton("ambient occlusion", debugShow == DebugShow::eSsao)) { 
            debugShow = DebugShow::eSsao;
        }
        if (ImGui::RadioButton("albedo", debugShow == DebugShow::eAlbedo)) { 
            debugShow = DebugShow::eAlbedo;
        }
        if (ImGui::RadioButton("emissive", debugShow == DebugShow::eEmissive)) { 
            debugShow = DebugShow::eEmissive;
        }
        if (ImGui::RadioButton("metallic roughness", debugShow == DebugShow::eMetallicRoughness)) { 
            debugShow = DebugShow::eMetallicRoughness;
        }
        if (ImGui::RadioButton("world positions", debugShow == DebugShow::eWorldPos)) { 
            debugShow = DebugShow::eWorldPos;
        }
        if (ImGui::RadioButton("normals", debugShow == DebugShow::eNormal)) { 
            debugShow = DebugShow::eNormal;
        }
        if (ImGui::RadioButton("shadow cascade 0", debugShow == DebugShow::eShadowCascade0)) { 
            debugShow = DebugShow::eShadowCascade0;
        }
        if (ImGui::RadioButton("shadow cascade 1", debugShow == DebugShow::eShadowCascade1)) { 
            debugShow = DebugShow::eShadowCascade1;
        }
        if (ImGui::RadioButton("debug cascades", debugShow == DebugShow::eDebugCascades)) { 
            debugShow = DebugShow::eDebugCascades;
        }
        ImGui::End();
    }

    class BackgroundWorker{
    public:
        using GpuJob = engine::graphics::BackgroundCommandContext::Command;
        using CpuJob = threads::WorkerTask;

        BackgroundWorker(engine::rhi::runtime::Rhi * api, engine::rhi::runtime::Device * device) : 
            _commandContext(&_worker, *api, *device, device->inFlightFramesCount()) {
            _worker.run();
        }

        ~BackgroundWorker() {
            destroy();
        }

        void destroy() {
            _commandContext.destroy();
        }

        void execute(std::vector<CpuJob> && tasks) {
            _worker.emplaceTasks(std::move(tasks));
        }

        void submit(std::vector<GpuJob> && tasks, auto && executor) {
            _commandContext.submit(std::move(tasks), std::forward<decltype(executor)>(executor));
        }

        threads::Worker & getScheduler() noexcept { return _worker; }

    private:
        threads::Worker _worker;
        engine::graphics::BackgroundCommandContext _commandContext;
    };

    std::optional<BackgroundWorker> backgroundgWorker;
    std::deque<BackgroundWorker::GpuJob> gpuTasks;
    std::deque<BackgroundWorker::CpuJob> cpuTasks;

    auto loadImageAsync(
        engine::rhi::runtime::Device & device, 
        engine::FileManager & fileManager, 
        engine::graphics::TextureManager & textureManager, 
        std::string imageKey, 
        std::string path, 
        engine::graphics::TextureFlags flags
    ) -> engine::graphics::ImageHandler * {
        using namespace engine;
        const auto & [imageHandler, isNew] = textureManager.addImage(imageKey);

        if (isNew) {
            gpuTasks.emplace_back(BackgroundWorker::GpuJob{[&device, &fileManager, &textureManager, imageKey = std::move(imageKey), path = std::move(path), flags]
                (engine::rhi::runtime::CommandList & commandList, std::vector<threads::WorkerTask> & callbacks) mutable {

                const auto imageBytes = fileManager.readFile(path);
                auto textureData = engine::graphics::parseTextureData(imageBytes, {
                        .flags = flags
                    }
                );

                const auto formatSupport = device.isFormatSupported(
                    textureData.metaInfo.format, 
                    rhi::FormatFeatureFlags::eFormatFeature_Sampled_Image
                );

                const bool sampledSupport = formatSupport.checkBit(rhi::FormatFeatureAccess::Bit::eOptimalTilingFeaturesBit);

                if (!sampledSupport) {
                    const bool srgb = static_cast<uint8_t>(flags & engine::graphics::TextureFlags::eAllowSrgbColorSpace) != 0u;
                    convertTextureDataFormat(srgb ? rhi::Format::eR8G8B8A8_Srgb : rhi::Format::eR8G8B8A8_Unorm, textureData);
                }

                auto image = textureManager.generateImage(std::move(textureData), commandList);

                callbacks.emplace_back([&textureManager, image = std::move(image), imageKey = std::move(imageKey)](){
                    textureManager.getImageHandler(imageKey)->image = std::move(image);
                });
            }});
        }

        return imageHandler;
    };

    uint32_t loadModelAsync(
        engine::rhi::runtime::Rhi & api,
        engine::rhi::runtime::Device & device, 
        threads::ExecutionContext * renderThread,
        engine::FileManager & fileManager,
        engine::graphics::ModelManager & modelManager,
        engine::graphics::TextureManager & textureManager,
        engine::scene::AnimationManager & animationManager,
        std::string key, 
        std::string folder,
        std::string file,
        std::move_only_function<void(uint32_t modelIndex)> loadCallback = nullptr
    ) {
        auto const [modelIdx, isNew] = modelManager.addModel2(key);
        if (isNew) {
            cpuTasks.emplace_back(threads::WorkerTask{[
                &api, &device, renderThread, 
                &fileManager, 
                &modelManager,
                &textureManager, 
                &animationManager,
                modelIdx, 
                key = std::move(key),
                folder = std::move(folder),
                file = std::move(file),
                loadCallback = std::move(loadCallback)
            ]() mutable {
                auto const modelBytes = fileManager.readFile(engine::fmt_string("%s%s", folder.c_str(), file.c_str()));
                auto modelLayout = engine::graphics::models::loadModelLayout(modelBytes, fileManager, folder);
                auto buffers = modelManager.loadModel(modelIdx, key, modelLayout);

                renderThread->emplaceTask(threads::WorkerTask{[
                    &api, &device, modelIdx, 
                    &fileManager, 
                    &modelManager,
                    &textureManager,
                    &animationManager,
                    buffers = std::move(buffers), 
                    modelLayout = std::move(modelLayout),
                    key = std::move(key),
                    loadCallback = std::move(loadCallback)
                ]() mutable {
                    //modelManager.setBuffers(modelIdx, std::move(buffers)); // ??
                    auto texturesToLoad = modelManager.prepareMaterials(key, modelIdx, modelLayout, textureManager);

                    gpuTasks.emplace_back(BackgroundWorker::GpuJob{[
                            &api, &device, 
                            &fileManager, 
                            &modelManager,
                            &textureManager, 
                            texturesToLoad = std::move(texturesToLoad), 
                            modelLayout = std::move(modelLayout),
                            modelIdx, 
                            buffers = std::move(buffers),
                            loadCallback = std::move(loadCallback)
                        ](
                            engine::rhi::runtime::CommandList & commandList, 
                            std::vector<threads::WorkerTask> & callbacks
                        ) mutable {
                        materialManager.createMaterialTextures(modelLayout, texturesToLoad, fileManager, textureManager, commandList);
                        modelManager.generateGpuBuffers(modelIdx, std::move(buffers), api, device, commandList); // ??
                        callbacks.emplace_back([
                            &modelManager,
                            modelIdx,
                            texturesToLoad = std::move(texturesToLoad), 
                            loadCallback = std::move(loadCallback)
                        ]() mutable {
                            modelManager.setDataReady(modelIdx, true);
                            for (auto & t : texturesToLoad) {
                                t.imageHandler->image = t.image;
                            }
                            if (loadCallback) {
                                loadCallback(modelIdx);
                            }
                        });
                    }});

                    const auto & modelData = modelManager.getModelData(modelIdx);
                    uint8_t i = 0u;
                    for (const auto & animation : modelData.animations) {
                        animationManager.registerAnimation(
                            engine::fmt_string("%s_anim%d", key.c_str(), i), 
                            engine::scene::SkeletalAnimation(animation)
                        );
                        ++i;
                    }
                }});
            }});
        }

        return modelIdx;
    }

    engine::scene::ViewParams viewParams;
    bool viewParamsUpdated = false;

    struct Stats {
        float cpuFrameTime = 0.0f;
        float gpuFrameTime = 0.0f;
        uint8_t inFlightFramesCount = 1u;
        uint32_t drawCalls = 0u;
        uint32_t dispatchCalls = 0u;
        uint16_t fps = 0u;
        uint16_t width = 0u;
        uint16_t height = 0u;
        uint8_t activeThreads = 0u;
    };

    Stats gStats;

    class SceneEventHandler final : public engine::InputObserver {
        inline static const auto kPlaneXOY = engine::vec4f(0.0f, 0.0f, 1.0f, 0.0f);
        float x0 = 0.0f;
        float y0 = 0.0f;
    public:
        bool onInputPointerEvent(const engine::PointerEvent & event) override {
            static bool pressedLeft = false;
            static bool pressedRight = false;

            switch (event.state) {
            case engine::InputEventState::ePress:
            {
                switch(event.button) {
                case engine::PointerButton::eButton0:
                {
                    pressedLeft = true;
                }
                    break;
                case engine::PointerButton::eButton1:
                    pressedRight = true;
                    break;
                }
            }
                break;
            case engine::InputEventState::eRelease:
            {
                switch(event.button) {
                case engine::PointerButton::eButton0:
                    pressedLeft = false;
                    break;
                case engine::PointerButton::eButton1:
                    pressedRight = false;
                    break;
                }
            }
                break;
            case engine::InputEventState::eNone:
            {
                if (pressedRight) {
                    constexpr float kRotationSpeed = 0.005f;
                    const auto rotation = engine::vec3f(kRotationSpeed * (y0 - event.y), 0.0f, kRotationSpeed * (event.x - x0));
                    auto resultRotation = controller.getRotation() + rotation;
                    resultRotation.x = std::clamp(resultRotation.x, -1.50f, 1.50f);
                    controller.setRotation(resultRotation);
                }

                if (pressedLeft) {
                    const auto ray0 = camera.screenToWorld({x0, y0});
                    const auto ray1 = camera.screenToWorld({event.x, event.y});
                    const auto worldPoint0 = engine::intersectionLinePlane(ray0.begin, ray0.end, kPlaneXOY);
                    const auto worldPoint1 = engine::intersectionLinePlane(ray1.begin, ray1.end, kPlaneXOY);
                    
                    controller.setPosition(controller.getPosition() + worldPoint0 - worldPoint1);
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

            controller.calculateTransform();
            const auto ray0 = camera.screenToWorld({x0, y0});
            const auto worldPoint0 = engine::intersectionLinePlane(ray0.begin, ray0.end, kPlaneXOY);

            const auto d = controller.getDistance();
            controller.setDistance(d - (dy / std::max(std::pow(2.0f, (8.0f - d)), 1.0f)));

            //
            controller.calculateTransform();
            const auto ray1 = camera.screenToWorld({x0, y0});
            const auto worldPoint1 = engine::intersectionLinePlane(ray1.begin, ray1.end, kPlaneXOY);

            controller.setPosition(controller.getPosition() + worldPoint0 - worldPoint1);

            return true;
        }

        bool onInpuKeyEvent(const engine::KeyEvent & event, const engine::InputKeyMod & mod) override {
            if (event.state == engine::InputEventState::ePress && event.key == engine::KeyboardKey::eK) {
                // threads::StopSource2 stopSource;
                // https://maptiler.com/{z}/{x}/{y}.pbf?key=YOUR_API_KEY
                curlNetworkService.sendRequest(backgroundgWorker->getScheduler(),
                    //engine::network::http::Request{"https://i.pinimg.com/originals/97/3e/fd/973efd74e4edfa29b2e939c6a8774c4e.jpg"},
                    // {z}/{x}/{y}
                    engine::network::http::Request{"https://api.maptiler.com/tiles/terrain-rgb/0/0/0.png?key=6ks89yREYo9GEfWO2i8h"},
                    [](std::vector<std::byte> && response, engine::network::RequestResult resultCode) {
                        if (resultCode == engine::network::RequestResult::eSuccess) {
                            // fileManager.getFileSystem("common_fs0")->writeFile("download/image.jpg", response.data(), response.size());
                            fileManager.getFileSystem("common_fs0")->writeFile("download/tile_0_0_0.png", response.data(), response.size());
                        }
                        printf("result code %d\n", static_cast<uint8_t>(resultCode));
                        //std::string_view str{reinterpret_cast<char*>(response.data()), response.size()};
                        //printf("resonse: %s\n", str.data());
                }, {}/*stopSource.get_token()*/);

                curlNetworkService.sendRequest(backgroundgWorker->getScheduler(),
                    engine::network::http::Request{"https://api.maptiler.com/tiles/v4/0/0/0.pbf?key=6ks89yREYo9GEfWO2i8h"},
                    [](std::vector<std::byte> && response, engine::network::RequestResult resultCode) {
                        if (resultCode == engine::network::RequestResult::eSuccess) {
                            //!!! data can be compressed !!! - need uncompress
                            std::string str{reinterpret_cast<char*>(response.data()), response.size()};
                            vtzero::vector_tile tile{str};

                            // define a variant covering the valid Vector Tile property types
                            using variant_type = std::variant<std::string, float, double, int64_t, uint64_t, bool>;
                            auto stringify = [](vtzero::property_value pvalue)->std::string {
                                variant_type v = vtzero::convert_property_value<variant_type>(pvalue);
                                return variant::visit(
                                    v, 
                                    [](std::string & value) { return value; },
                                    [](auto && value) { return std::to_string(value); }
                                );
                            };

                            struct GeometryHandler {
                                void points_begin(uint32_t count) { printf("Points [%d]", count); }
                                void points_point(const vtzero::point p) { printf("(%d, %d), ", p.x, p.y); }
                                void points_end() { printf("\n"); }

                                void linestring_begin(uint32_t count) { printf("Line [%d]", count); }
                                void linestring_point(const vtzero::point p) { printf("(%d, %d), ", p.x, p.y); }
                                void linestring_end() { printf("\n"); }

                                void ring_begin(uint32_t count) { printf("Polygon Ring [%d]", count); }
                                void ring_point(const vtzero::point p) { printf("(%d, %d), ", p.x, p.y); }
                                void ring_end(vtzero::ring_type) { printf("\n"); }
                            };

                            while (auto layer = tile.next_layer()) { //
                                printf("Layer: %s\n", layer.name().to_string().c_str());

                                // 5. Iterate over features within the layer
                                while (auto feature = layer.next_feature()) { //
                                    printf("Feature ID: %lu\n", feature.id());

                                    // 6. Read Feature Tags (Metadata attributes)
                                    printf("Properties:\n");
                                    while (auto property = feature.next_property()) {
                                         printf("%s = %s\n",property.key().to_string().c_str(), stringify(property.value()).c_str());
                                    }

                                    // 7. Decode Geometries (Points, Lines, or Polygons)
                                    GeometryHandler handler;
                                    vtzero::decode_geometry(feature.geometry(), handler);
                                }
                            }
                            
                            fileManager.getFileSystem("common_fs0")->writeFile("download/tile_0_0_0.pbf", response.data(), response.size());
                        }
                        printf("result code %d\n", static_cast<uint8_t>(resultCode));
                }, {}/*stopSource.get_token()*/);



                /*stopSource.request_stop();*/
            }
            return false;
        }

        bool onInpuCharEvent(uint16_t code) override { 
            return false;
        }
    } sceneEventHandler;

    Example::Example() {
        using namespace engine;
        
        fileManager.createFileSystem<CommonFileSystem>("common_fs0", CommonFileSystem::getCurrentDirectory() + "./resources/");

        addInputObserver(&_imgui);
        addInputObserver(&sceneEventHandler);

        controller.setDistance(10.0f);
        controller.setMinMaxDistance(0.01f, 30.0f);
        controller.setRotation({-1.3f, 0.0f, 0.5f});

        camera.makePerspective(std::numbers::pi_v<float> / 3.0f, 16.0f / 9.0f, kNearFarPlane.x, kNearFarPlane.y);
    }

    Example::~Example() = default;

    void Example::destroy() {
        backgroundgWorker->destroy();

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
        _externalCommandLists.clear();

        _queryPool = nullptr;

        scene->destroy();

        _shaderModulesManager = nullptr;
        _imgui.destroy();
        drawHelper.destroy();
        
        animationManager.destroy();
        modelManager->destroy();
        materialManager.destroy();
        textureManager->destroy();

        ssao.destroy();

        shadowMapUniformBuffer = nullptr;
        shadowMap = nullptr;

        skyBoxCubeMapImage = nullptr;
        
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
        _gpuFeatures.features.independentBlend = true;

        auto gpuDevices = _api->enumerateDevices({
            _gpuFeatures, 
            {
                engine::vulkan::runtime::Rhi::Api::Impl::kSwapchainExtName, 
                VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
                VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME, // for Resetting the query pool
                VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME // for use gl_Layer into vertex shaders (since Vulkan 1.2 support natively)
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
        deviceCreateInfo.commandPoolThreadsCount = 2u;
        deviceCreateInfo.inFlightFramesCount = 2u;

        _device = _api->createDevice(_gpu, deviceCreateInfo);

        const bool isSupportShaderIndexLayer = _device->isExtensionSupported(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME);

        backgroundgWorker.emplace(_api.get(), _device.get());
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
        const auto framesCount = _device->inFlightFramesCount();

        auto const size = _swapchain->getSize();

        const auto depthFormat = _device->getSupportedDepthFormat(kDepthFormatTargetBits, false);

        auto imageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment | engine::rhi::ImageUsageFlags::eTransient_Attachment,
            depthFormat, {size.width, size.height, 1u}
        };
        imageInfo.memoryFlags = engine::rhi::MemoryPropertyFlags::eDeviceLocal | engine::rhi::MemoryPropertyFlags::eLazilyAllocated;
        imageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        _depthImages.clear();
        _depthImages.reserve(framesCount);
        for (uint32_t i = 0u; i < framesCount; ++i) {
            _depthImages.emplace_back(std::move(_api->createImage(*_device, imageInfo)));
        }

        //
        auto colorImageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eColor_Attachment | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR8G8B8A8_Unorm, {size.width, size.height, 1u}
        }; 
        colorImageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        _colorAttachmentImages.clear();
        _colorAttachmentImages.reserve(framesCount);
        for (uint32_t i = 0u; i < framesCount; ++i) {
            _colorAttachmentImages.emplace_back(std::move(_api->createImage(*_device, colorImageInfo)));
        }

        _sceneFrameBuffers.clear();
        _sceneFrameBuffers.reserve(framesCount);
        for (uint32_t i = 0u; i < framesCount; ++i) {
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

        // ssao:
        ssao.resize(*_api, *_device, framesCount, size.width, size.height);

        // gBuffer attachments
        _gBufferAttachments.clear();
        _gBufferAttachments.reserve(framesCount);

        auto positionImageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eColor_Attachment | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR32_Sfloat, {size.width, size.height, 1u}
        };
        positionImageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        auto colorImageInfo565 = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eColor_Attachment | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR5G6B5_Unorm_Pack16, {size.width, size.height, 1u}
        };
        colorImageInfo565.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        auto colorImageInfo8888 = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eColor_Attachment | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR8G8B8A8_Unorm, {size.width, size.height, 1u}
        };
        colorImageInfo8888.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        auto depthImageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment | engine::rhi::ImageUsageFlags::eTransient_Attachment,
            depthFormat, {size.width, size.height, 1u}
        };
        depthImageInfo.memoryFlags = engine::rhi::MemoryPropertyFlags::eDeviceLocal | engine::rhi::MemoryPropertyFlags::eLazilyAllocated;
        depthImageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        auto metallicRoughnessColorImageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eColor_Attachment | engine::rhi::ImageUsageFlags::eSampled,
            engine::rhi::Format::eR8G8_Unorm, {size.width, size.height, 1u}
        };
        metallicRoughnessColorImageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        for (uint32_t i = 0u; i < framesCount; ++i) {
            _gBufferAttachments.emplace_back(
                _api->createImage(*_device, positionImageInfo),                 // position linear depth
                _api->createImage(*_device, colorImageInfo8888),                // normal
                _api->createImage(*_device, colorImageInfo8888),                // albedo
                _api->createImage(*_device, colorImageInfo8888),                // emissive
                _api->createImage(*_device, colorImageInfo8888),                // emissive2
                _api->createImage(*_device, depthImageInfo),                    // depth
                _api->createImage(*_device, metallicRoughnessColorImageInfo)    // metallicRoughness
            );
        }

        // gBuffer framebuffers
        _gBuffers.clear();
        _gBuffers.reserve(framesCount);

        for (uint32_t i = 0u; i < framesCount; ++i) {
            {
                std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{6u};

                auto & positionImage = _gBufferAttachments[i].position;
                fbAttachemnts[0u].image = positionImage.get();
                fbAttachemnts[0u].imageSize = {positionImage->getExtent().width, positionImage->getExtent().height};
                fbAttachemnts[0u].aspect = engine::rhi::ImageAspectFlags::eColor;

                auto & normalImage = _gBufferAttachments[i].normal;
                fbAttachemnts[1u].image = normalImage.get();
                fbAttachemnts[1u].imageSize = {normalImage->getExtent().width, normalImage->getExtent().height};
                fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eColor;

                auto & albedoImage = _gBufferAttachments[i].albedo;
                fbAttachemnts[2u].image = albedoImage.get();
                fbAttachemnts[2u].imageSize = {albedoImage->getExtent().width, albedoImage->getExtent().height};
                fbAttachemnts[2u].aspect = engine::rhi::ImageAspectFlags::eColor;

                auto & emissiveImage = _gBufferAttachments[i].emissive;
                fbAttachemnts[3u].image = emissiveImage.get();
                fbAttachemnts[3u].imageSize = {emissiveImage->getExtent().width, emissiveImage->getExtent().height};
                fbAttachemnts[3u].aspect = engine::rhi::ImageAspectFlags::eColor;

                auto & metallicRoughnessImage = _gBufferAttachments[i].metallicRoughness;
                fbAttachemnts[4u].image = metallicRoughnessImage.get();
                fbAttachemnts[4u].imageSize = {metallicRoughnessImage->getExtent().width, metallicRoughnessImage->getExtent().height};
                fbAttachemnts[4u].aspect = engine::rhi::ImageAspectFlags::eColor;

                auto & depthImage = _gBufferAttachments[i].depth;
                fbAttachemnts[5u].image = depthImage.get();
                fbAttachemnts[5u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
                fbAttachemnts[5u].aspect = engine::rhi::ImageAspectFlags::eDepth;

                _gBuffers.emplace_back(_api->createFrameBuffer(*_device, std::move(fbAttachemnts), _renderPasses[kGbufferRenderPass]));
            }
        }

        _emissiveBluredBuffers.clear();
        _emissiveBluredBuffers.reserve(framesCount * 2u);
        for (uint32_t i = 0u; i < framesCount; ++i) {
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{1u};
            auto & image = _gBufferAttachments[i].emissive;
            fbAttachemnts[0u].image = image.get();
            fbAttachemnts[0u].imageSize = {image->getExtent().width, image->getExtent().height};

            _emissiveBluredBuffers.emplace_back(_api->createFrameBuffer(*_device, std::move(fbAttachemnts), _renderPasses[kBlurRenderPass]));
        }

        for (uint32_t i = 0u; i < framesCount; ++i) {
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{1u};
            auto & image = _gBufferAttachments[i].emissive2;
            fbAttachemnts[0u].image = image.get();
            fbAttachemnts[0u].imageSize = {image->getExtent().width, image->getExtent().height};

            _emissiveBluredBuffers.emplace_back(_api->createFrameBuffer(*_device, std::move(fbAttachemnts), _renderPasses[kBlurRenderPass]));
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
        gStats.inFlightFramesCount = _device->inFlightFramesCount();
        gStats.width = w;
        gStats.height = h;
        gStats.activeThreads = threads::gThreadsCount.load(std::memory_order_acquire);
    }

    void Example::createRenderPasses() {
        const auto bytes = fileManager.readFile("descriptors/render_pass.json");

        engine::json::Json json{};
        std::string_view input(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        const auto err = engine::json::read_json(json, input);

        const auto depthFormat = _device->getSupportedDepthFormat(kDepthFormatTargetBits, false);
        const auto formats0 = std::array{engine::rhi::Format::eB8G8R8A8_Unorm};
        const auto formats1 = std::array{engine::rhi::Format::eR8G8B8A8_Unorm, depthFormat};

        _renderPasses.emplace_back(_api->getRenderPass(*_device, engine::parseRenderPass(json["screen"], formats0))); // kScreenPass
        _renderPasses.emplace_back(_api->getRenderPass(*_device, engine::parseRenderPass(json["scene"], formats1))); // kScenePass

        { // gbuffer
            const auto formatsGBuffer = std::array {
                engine::rhi::Format::eR32_Sfloat,           // position linear depth
                engine::rhi::Format::eR8G8B8A8_Unorm,  // normal
                engine::rhi::Format::eR8G8B8A8_Unorm,       // albedo
                engine::rhi::Format::eR8G8B8A8_Unorm,       // emissive
                engine::rhi::Format::eR8G8_Unorm,           // metalicRoughness
                depthFormat
            };

            _renderPasses.emplace_back(_api->getRenderPass(*_device, engine::parseRenderPass(json["gBuffer"], formatsGBuffer))); // kGbufferRenderPass
        }

        { // blur
            const auto formatsEmissiveBlur = std::array{engine::rhi::Format::eR8G8B8A8_Unorm};
            _renderPasses.emplace_back(_api->getRenderPass(*_device, engine::parseRenderPass(json["blur"], formatsEmissiveBlur))); // kBlurRenderPass
        }

        // ssao
        ssao.createRenderPass(*_api, *_device);
    }

    void Example::loadShaders() {
        _shaderModulesManager = _api->createShaderModulesManager();

        { // kImguiProgram
            auto const vBytes = fileManager.readFile("shaders/imgui.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/imgui.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("imgui.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("imgui.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
        { // kPostEffectProgram
            auto const vBytes = fileManager.readFile("shaders/postEffect.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/postEffect.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("postEffect.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("postEffect.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kDebugProgram
            auto const vBytes = fileManager.readFile("shaders/debug.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/debug.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("debug.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("debug.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kSpritesProgram
            auto const vBytes = fileManager.readFile("shaders/sprite.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/sprite.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("sprite.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("sprite.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kMeshProgram
            auto const vBytes = fileManager.readFile("shaders/mesh.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/mesh.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        // { // kMeshSkeletalInstancedProgram
        //     auto const vBytes = fileManager.readFile("shaders/mesh_skeletal_instanced.vert.spv");
        //     auto const fBytes = fileManager.readFile("shaders/mesh.frag.spv");

        //     const auto & vsModule = _shaderModulesManager->registerModule("mesh_skeletal_instanced.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
        //     const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

        //     _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}, {.dynamicDescriptorsMask = 0x0u}));
        // }

        { // kMeshInstancedProgram
            auto const vBytes = fileManager.readFile("shaders/mesh_instanced.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/mesh.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh_instanced.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kComputeProgram
            auto const cBytes = fileManager.readFile("shaders/mesh_skin_compute.comp.spv");
            const auto & compModule = _shaderModulesManager->registerModule("mesh_skin_compute.comp", *_device, engine::rhi::shaders::Stages::eCompute, cBytes);
            _programs.emplace_back(_api->createProgram(*_device, std::array{compModule.getView()}));
        }

        { // kMeshAfterComputeProgram
            auto const vBytes = fileManager.readFile("shaders/mesh_after_compute.vert.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh_after_compute.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh.frag", *_device, engine::rhi::shaders::Stages::eFragment, {});

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kSsaoProgram
            auto const vBytes = fileManager.readFile("shaders/ssao.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/ssao.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("ssao.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("ssao.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kEmissiveBlurProgram
            auto const vBytes = fileManager.readFile("shaders/gaussblur.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/gaussblur.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("gaussblur.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("gaussblur.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kMeshShadowProgram
            auto const vBytes = fileManager.readFile("shaders/mesh_instanced_shadow.vert.spv");
            const auto & vsModule = _shaderModulesManager->registerModule("mesh_instanced_shadow.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView()}));
        }

        { // kMeshAfterComputeShadowProgram
            auto const vBytes = fileManager.readFile("shaders/mesh_after_compute_shadow.vert.spv");
            const auto & vsModule = _shaderModulesManager->registerModule("mesh_after_compute_shadow.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView()}));
        }

        { // kMeshInstancedProgramNoPbr
            auto const vBytes = fileManager.readFile("shaders/mesh_instanced2.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/mesh2.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh_instanced2.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh2.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kMeshAfterComputeProgramNoPbr
            auto const vBytes = fileManager.readFile("shaders/mesh_after_compute2.vert.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("mesh_after_compute2.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("mesh2.frag", *_device, engine::rhi::shaders::Stages::eFragment, {});

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }

        { // kSpritesProgram2
            auto const vBytes = fileManager.readFile("shaders/sprite.vert.spv");
            auto const fBytes = fileManager.readFile("shaders/sprite2.frag.spv");

            const auto & vsModule = _shaderModulesManager->registerModule("sprite.vert", *_device, engine::rhi::shaders::Stages::eVertex, vBytes);
            const auto & fsModule = _shaderModulesManager->registerModule("sprite2.frag", *_device, engine::rhi::shaders::Stages::eFragment, fBytes);

            _programs.emplace_back(_api->createProgram(*_device, std::array{vsModule.getView(), fsModule.getView()}));
        }
    }
    
    void Example::createPipelines() {
        using namespace engine::rhi;

        //
        modelManager.emplace(materialManager);
        textureManager.emplace(*_api, *_device);
        graphicsService.emplace(*_api, *_device, *textureManager, *modelManager, materialManager);
        scene.emplace(*graphicsService, drawHelper);
        //

        loadShaders();
        _imgui.createPipeline(*_api, *_device, _programs[kImguiProgram], _renderPasses[kScreenRenderPass], 0u);
        drawHelper.createPipelines(
            *_api, *_device, 
            _programs[kDebugProgram], _renderPasses[kGbufferRenderPass], 
            5u, // attachmentsCount 
            2u, // colorAttachmentIdx
            0u, true);

        const auto bytes = fileManager.readFile("descriptors/pipelines.json");
        std::string_view input(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        engine::json::Json json{};
        const auto err = engine::json::read_json(json, input);

        // kPostEffectPipeline
        const GraphicsPipelineStatesInfo info0 = engine::parseGraphicsPipelineInfo(json["postEffect"]);
        _pipelines.emplace_back(_api->createGraphicsPipeline(*_device, _renderPasses[kScreenRenderPass], _programs[kPostEffectProgram], info0, 0u));

        // kSsaoPipeline
        _pipelines.emplace_back(_api->createGraphicsPipeline(*_device, ssao.getRenderPass(), _programs[kSsaoProgram], info0, 0u));

        // kEmissiveBlurPipeline
        _pipelines.emplace_back(_api->createGraphicsPipeline(*_device, _renderPasses[kBlurRenderPass], _programs[kEmissiveBlurProgram], info0, 0u));

        // scene pipelines
        const GraphicsPipelineStatesInfo info2 = engine::parseGraphicsPipelineInfo(json["sprite_common"]);
        scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kGbufferRenderPass], _programs[kSpritesProgram], info2, 0u));
        
        // skeletal mesh
        // const GraphicsPipelineStatesInfo info4 = engine::parseGraphicsPipelineInfo(json["mesh_skeletal"]);
        // scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kSceneRenderPass], _programs[kMeshSkeletalInstancedProgram], info4, 0u));
        
        // mesh
        const GraphicsPipelineStatesInfo info5 = engine::parseGraphicsPipelineInfo(json["mesh"]);
        scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kGbufferRenderPass], _programs[kMeshInstancedProgram], info5, 0u));
        scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kGbufferRenderPass], _programs[kMeshInstancedProgramNoPbr], info5, 0u));

        // compute
        scene->registerComputePipeline(_api->createComputePipeline(*_device, _programs[kComputeProgram]));

        const GraphicsPipelineStatesInfo info6 = engine::parseGraphicsPipelineInfo(json["mesh_skeletal_computed"]);
        scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kGbufferRenderPass], _programs[kMeshAfterComputeProgram], info6, 0u));
        scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kGbufferRenderPass], _programs[kMeshAfterComputeProgramNoPbr], info6, 0u));

        // shadow
        shadowMap = std::make_unique<engine::graphics::CascadeShadowMap>(*_api, *_device, 4096u, 2u, 16u, kNearFarPlane.x, kNearFarPlane.y);
        shadowMap->initCascadeSplits({0.75f, 0.25f, -87.0f});
        shadowMap->createRenderData();
        shadowMap->setDirection(glm::normalize(lightDir));

        const GraphicsPipelineStatesInfo info7 = engine::parseGraphicsPipelineInfo(json["mesh_shadows"]);
        const uint32_t meshShadowPipeline = scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, shadowMap->getRenderPass(), _programs[kMeshShadowProgram], info7, 0u));
        scene->assignCommonPipeline(meshShadowPipeline, engine::scene::Scene::CommonPipeline::Type::eMeshShadow);

        const GraphicsPipelineStatesInfo info8 = engine::parseGraphicsPipelineInfo(json["mesh_skin_shadows"]);
        const uint32_t meshSkinShadowPipeline = scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, shadowMap->getRenderPass(), _programs[kMeshAfterComputeShadowProgram], info8, 0u));
        scene->assignCommonPipeline(meshSkinShadowPipeline, engine::scene::Scene::CommonPipeline::Type::eMeshSkinnedShadow);

        // sprite2
        scene->registerGraphicsPipeline(_api->createGraphicsPipeline(*_device, _renderPasses[kGbufferRenderPass], _programs[kSpritesProgram2], info2, 0u));
    }

    void Example::createDefaultResources() {
        using namespace engine::rhi;
        using namespace engine::graphics;

        auto * commandPool = _device->acquireCommandPool(engine::rhi::queue_flags::transfrer);
        auto tmpCommandList = _api->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary);

        tmpCommandList->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        // first reset _queryPool, before use it
        const auto inflightFrames = _device->inFlightFramesCount();
        for (size_t i = 0u; i < inflightFrames; ++i) {
            tmpCommandList->resetQueryPool(_queryPool.get(), i * 2u, 2u);
        }

        // textureManager resources
        textureManager->createDefaultResources(tmpCommandList);

        ////
        // test cubemap
        // const auto imageBytes0 = fileManager.readFile("assets/skybox/back.jpg");
        const auto imageBytes0 = fileManager.readFile("assets/interrior/2.jpg");
        auto textureData0 = engine::graphics::parseTextureData(imageBytes0, {
                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
            }
        );

        // const auto imageBytes1 = fileManager.readFile("assets/skybox/front.jpg");
        const auto imageBytes1 = fileManager.readFile("assets/interrior/0.jpg");
        auto textureData1 = engine::graphics::parseTextureData(imageBytes1, {
                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
            }
        );

        // const auto imageBytes2 = fileManager.readFile("assets/skybox/right.jpg");
        const auto imageBytes2 = fileManager.readFile("assets/interrior/6.jpg");
        auto textureData2 = engine::graphics::parseTextureData(imageBytes2, {
                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
            }
        );

        // const auto imageBytes3 = fileManager.readFile("assets/skybox/left.jpg");
        const auto imageBytes3 = fileManager.readFile("assets/interrior/5.jpg");
        auto textureData3 = engine::graphics::parseTextureData(imageBytes3, {
                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
            }
        );

        // const auto imageBytes4 = fileManager.readFile("assets/skybox/up.jpg");
        const auto imageBytes4 = fileManager.readFile("assets/interrior/4.jpg");
        auto textureData4 = engine::graphics::parseTextureData(imageBytes4, {
                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
            }
        );

        // const auto imageBytes5 = fileManager.readFile("assets/skybox/down.jpg");
        const auto imageBytes5 = fileManager.readFile("assets/interrior/3.jpg");
        auto textureData5 = engine::graphics::parseTextureData(imageBytes5, {
                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
            }
        );

        engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Srgb, textureData0);
        engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Srgb, textureData1);
        engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Srgb, textureData2);
        engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Srgb, textureData3);
        engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Srgb, textureData4);
        engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Srgb, textureData5);

        std::array<engine::graphics::TextureLayerData, 6u> layers;
        layers[0u] = std::move(textureData0.data);
        layers[1u] = std::move(textureData1.data);
        layers[2u] = std::move(textureData2.data);
        layers[3u] = std::move(textureData3.data);
        layers[4u] = std::move(textureData4.data);
        layers[5u] = std::move(textureData5.data);

        skyBoxCubeMapImage = textureManager->generateImage(engine::rhi::Format::eR8G8B8A8_Srgb, textureData0.metaInfo.size, engine::graphics::TextureFlags::eNone, layers, 
            engine::rhi::ImageCreateFlags::eCube_Compatible, *tmpCommandList);

        const auto & [imageHandler, isNew] = textureManager->addImage("skyBoxCubeMapImage");
        textureManager->getImageHandler("skyBoxCubeMapImage")->image = skyBoxCubeMapImage;
        ////

        //
        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "model", 
                "assets/models/",
                "model.glb",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 1u, 
                        .skeletalPipeline = 3u
                    };

                    auto const animId = animationManager.getAnimationIndex("model_anim0");
                    for (size_t i = 0u; i < 500u; ++i) {
                        auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                            engine::scene::Bounds::Type::eCube, pipelines, 
                            {.size = {0.47f, 0.47f, 0.47f}, .type = engine::scene::ObjectSizeLayout::Type::eMin},
                            i == 5u ? -1 : 0u);
                        
                        // meshRoot->childAt(0u)->getAttachment(1u)->setHidden(true);

                        meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, engine::common::random(-3.14f, 3.14f)));
                        meshRoot->setPosition(engine::vec3f(-2.0f + i % 20 * 0.5f, 0.0f + i / 20 * -0.5f, 0.0f));

                        if (auto * skeleton = static_cast<engine::scene::Skeleton *>(meshRoot->getAttachment())) {
                            if (i == 5u) {
                                auto * handBone = skeleton->node(15u);
                                swordNode->removeFromParent();
                                handBone->addChild(swordNode);
                                meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, 0.25f));
                                // meshRoot->childAt(0u)->getAttachment(1u)->setHidden(true); // todo: fix it for compute skinning method
                            }

                            auto * anim0 = scene->createAnimInfo<engine::scene::SkeletalAnimation>(animId, engine::common::random(0.25f, 1.7f));
                            scene->destroyAnimInfo(skeleton->setAnimation(anim0));
                        }

                        sceneWorldRoot->addChild(meshRoot);
                    }
                }
            );
        }

        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "scene", 
                "assets/models/1/",
                "scene.gltf",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 2u, 
                        .skeletalPipeline = 4u
                    };

                    auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                            engine::scene::Bounds::Type::eCube, pipelines, 
                            {.size = {6.5f, 6.5f, 6.5f}, .type = engine::scene::ObjectSizeLayout::Type::eMin});

                    meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, 0.0f));
                    meshRoot->setPosition(engine::vec3f(0.0f, 4.0f, 0.0f));
                    sceneWorldRoot->addChild(meshRoot);
                }
            );
        }

        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "wheel", 
                "assets/models/",
                "Minsk_wheel.glb",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 2u, 
                        .skeletalPipeline = 4u
                    };
                    auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                        engine::scene::Bounds::Type::eCube, pipelines,
                        {.size = {3.5f, 3.5f, 3.5f}, .type = engine::scene::ObjectSizeLayout::Type::eMin});

                    if (auto * skeleton = static_cast<engine::scene::Skeleton *>(meshRoot->getAttachment())) {
                        auto const animId = animationManager.getAnimationIndex("wheel_anim0");
                        auto * anim = scene->createAnimInfo<engine::scene::SkeletalAnimation>(animId, engine::common::random(0.25f, 1.7f));
                        scene->destroyAnimInfo(skeleton->setAnimation(anim));
                    }

                    meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, 1.57f));
                    meshRoot->setPosition(engine::vec3f(-3.0f, 2.0f, 0.0f));
                    sceneWorldRoot->addChild(meshRoot);
                }
            );
        }

        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "buster_drone", 
                "assets/models/",
                "buster_drone.glb",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 1u, 
                        .skeletalPipeline = 3u
                    };
                    auto const animId = animationManager.getAnimationIndex("buster_drone_anim0");
                    for (uint8_t i = 0u; i < 20u; ++i) {
                        auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                            engine::scene::Bounds::Type::eCube, pipelines, 
                            {.size = {1.5f, 1.5f, 1.5f}, .type = engine::scene::ObjectSizeLayout::Type::eMin});

                        if (auto * skeleton = static_cast<engine::scene::Skeleton *>(meshRoot->getAttachment())) {
                            auto * anim = scene->createAnimInfo<engine::scene::SkeletalAnimation>(animId, engine::common::random(0.25f, 1.7f));
                            scene->destroyAnimInfo(skeleton->setAnimation(anim));
                        }

                        meshRoot->setRotation(engine::vec3f(engine::pi2, 0.0f, engine::common::random(-3.14f, 3.14f)));
                        meshRoot->setPosition(engine::vec3f(-5.5f + i % 2 * 1.5f, 0.0f - i / 2 * 1.5f, 0.4126f));
                        sceneWorldRoot->addChild(meshRoot);
                    }
                }
            );
        }

        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "chaman", 
                "assets/models/chaman/",
                "scene.gltf",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 1u, 
                        .skeletalPipeline = 3u
                    };
                    auto const animId = animationManager.getAnimationIndex("chaman_anim1");
                    for (uint16_t i = 0u; i < 500u; ++i) {
                        auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                            engine::scene::Bounds::Type::eCube, pipelines,
                            {.size = {0.47f, 0.47f, 0.47f}, .type = engine::scene::ObjectSizeLayout::Type::eMin});

                        if (auto * skeleton = static_cast<engine::scene::Skeleton *>(meshRoot->getAttachment())) {
                            auto * anim = scene->createAnimInfo<engine::scene::SkeletalAnimation>(animId, engine::common::random(0.25f, 1.7f));
                            scene->destroyAnimInfo(skeleton->setAnimation(anim));
                        }

                        meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, engine::common::random(-3.14f, 3.14f)));
                        meshRoot->setPosition(engine::vec3f(8.5f + i % 20 * 0.5f, 0.0f + i / 20 * -0.5f, 0.0f));
                        sceneWorldRoot->addChild(meshRoot);
                    }
                }
            );
        }

        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "cathedral", 
                "assets/models/",
                "St_Basils_Cathedral.glb",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 2u, 
                        .skeletalPipeline = 4u
                    };
                    auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                        engine::scene::Bounds::Type::eCube, pipelines,
                        {.size = {4.5f, 4.5f, 4.5f}, .type = engine::scene::ObjectSizeLayout::Type::eMin});

                    meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, 1.57f));
                    meshRoot->setPosition(engine::vec3f(10.0f, 4.0f, 0.0f));
                    sceneWorldRoot->addChild(meshRoot);
                }
            );
        }

        {
            auto const modelIdx = loadModelAsync(
                *_api, *_device,
                _renderThread,
                fileManager,
                *modelManager,
                *textureManager,
                animationManager,
                "windmill", 
                "assets/models/windmill/",
                "scene.gltf",
                [](uint32_t modelIndex) {
                    engine::scene::MeshPipelines pipelines = {
                        .commonPipeline = 1u, 
                        .skeletalPipeline = 3u
                    };
                    auto * meshRoot = engine::scene::createNodeWithSkeletalMesh(*scene, *modelManager, modelIndex, 
                        engine::scene::Bounds::Type::eCube, pipelines,
                        {.size = {3.5f, 3.5f, 3.5f}, .type = engine::scene::ObjectSizeLayout::Type::eMin});

                    if (auto * skeleton = static_cast<engine::scene::Skeleton *>(meshRoot->getAttachment())) {
                        auto const animId = animationManager.getAnimationIndex("windmill_anim0");
                        auto * anim = scene->createAnimInfo<engine::scene::SkeletalAnimation>(animId, engine::common::random(0.25f, 1.7f));
                        scene->destroyAnimInfo(skeleton->setAnimation(anim));
                    }

                    meshRoot->setRotation(engine::vec3f(1.57f, 0.0f, 0.0f));
                    meshRoot->setPosition(engine::vec3f(-7.0f, 2.0f, -0.1f));
                    sceneWorldRoot->addChild(meshRoot);
                }
            );
        }

        textureManager->generateGpuData(tmpCommandList);
        //

        {
            // atlas test
            const auto imageBytes = fileManager.readFile("assets/atlases/hero-pma.png");
            const auto textureData = engine::graphics::parseTextureData(imageBytes, {
                                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
                            });

            using namespace engine::rhi;
            const auto imageInfo = ImageInfo{
                ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
                textureData.metaInfo.format, {textureData.metaInfo.size.x, textureData.metaInfo.size.y, 1u}
            };

            auto image = _api->createImage(*_device, imageInfo);
            const auto stagingSize = textureData.data[0u].size();
            engine::graphics::StagingBuffer staging{*_api, *_device, stagingSize};
            
            auto * imgData = staging.map();
            std::memcpy(imgData, textureData.data[0u].data(), stagingSize);
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

            auto const atlasBytes = fileManager.readFile("assets/atlases/hero-pma.atlas");
            auto frames = engine::graphics::FramesParser::loadFramesSpine(atlasBytes, {0u, 0u}, "hero_");

            textureManager->addFrames(frames, image);
        }

        if (1) {
            // atlas test2
            // auto const imageBytes = fileManager.readFile("assets/atlases/fire2.png");
            const auto imageBytes = fileManager.readFile("assets/atlases/explosion2.png");
            const auto textureData = engine::graphics::parseTextureData(imageBytes, {
                                .flags = engine::graphics::TextureFlags::eAllowSrgbColorSpace
                            }
                        );

            using namespace engine::rhi;
            const auto imageInfo = ImageInfo{
                ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
                textureData.metaInfo.format, {textureData.metaInfo.size.x, textureData.metaInfo.size.y, 1u}
            };

            auto image = _api->createImage(*_device, imageInfo);
            const auto stagingSize = textureData.data[0u].size();
            engine::graphics::StagingBuffer staging{*_api, *_device, stagingSize};
            
            auto * imgData = staging.map();
            std::memcpy(imgData, textureData.data[0u].data(), stagingSize);
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
            // auto frames = engine::graphics::FramesParser::generateFrames({6u, 6u}, "explosion_");
            textureManager->addFrames(frames, image);

            {
                std::vector<std::string> animFrames;
                animFrames.reserve(48u);
                for (size_t i = 0u; i < 48u; ++i) {
                    animFrames.emplace_back(engine::fmt_string("explosion_%d", i));
                }

                // animFrames.reserve(36u);
                // for (size_t i = 0u; i < 36u; ++i) {
                //     animFrames.emplace_back(engine::fmt_string("explosion_%d", i));
                // }

                animationManager.registerAnimation("anim0", engine::scene::FramesSequence(*textureManager, animFrames));
            }
        }

        //
        ssao.createConstResources(*_api, *_device, *tmpCommandList, 
            {
                .kernelSize = kSsaoKernelSize,
                .noiseDim = kSsaoNoiseDim
            }
        );
        //

        // create post buffers and default image
        BufferInfo bufferInfo {
            BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
            MemoryPropertyFlags::eDeviceLocal
        };

        _postPositions = _api->createBuffer(*_device, bufferInfo, sizeof(std::array<float, 2u>) * 4u);

        bufferInfo.usage = BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst;
        _postIndices = _api->createBuffer(*_device, bufferInfo, sizeof(uint16_t) * 6u);

        // staging buffer for vtx, idx, image;
        constexpr auto stagingSize = sizeof(std::array<float, 2u>) * 4u + sizeof(uint16_t) * 6u + sizeof(uint32_t) * 4u;
        StagingBuffer staging{*_api, *_device, stagingSize};
        
        std::byte * memory = staging.map();
        const auto vtxOffset = staging.requestBytes(sizeof(std::array<float, 2u>) * 4u);
        const auto idxOffset = staging.requestBytes(sizeof(uint16_t) * 6u);

        auto * vtxData = reinterpret_cast<std::array<float, 2u>*>(memory + vtxOffset);
        auto * idxData = reinterpret_cast<uint16_t*>(memory + idxOffset);

        vtxData[0] = {-1.0, -1.0}; vtxData[1] = {-1.0, 1.0};
        vtxData[2] = {1.0, 1.0}; vtxData[3] = {1.0, -1.0}; 
        
        idxData[0] = 0u; idxData[1] = 1u; idxData[2] = 2u;
        idxData[3] = 0u; idxData[4] = 2u; idxData[5] = 3u;

        staging.flush();
        staging.unmap();
        // staging buffer works complete

        tmpCommandList->copyBuffer2Buffer(staging.getBuffer(), _postPositions, 
            {.srcOffset = vtxOffset, .dstOffset = 0u, .size = _postPositions->sizeInBytes()});
        tmpCommandList->copyBuffer2Buffer(staging.getBuffer(), _postIndices, 
            {.srcOffset = idxOffset, .dstOffset = 0u, .size = _postIndices->sizeInBytes()});
        
        tmpCommandList->end();
        tmpCommandList->record(0u);

        auto lists = std::array{tmpCommandList.get()};
        auto fence = _api->createFence(*_device, FenceCreateFlags::eUnsignaled);
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

        // scene
        if (true) {
            using namespace engine;
            sceneWorldRoot = scene->createNode();
            scene->rootNode()->addChild(sceneWorldRoot);

            auto m0 = engine::makeMatrix(0.5F);
            engine::rotateMatrix_z(m0, 1.57f);
            m0[3] = {1.0f, 0.0f, 0.0f, 1.0f};

            auto m1 = engine::makeMatrix(0.75F);
            m1[3] = {1.5f, 0.0f, 0.0f, 1.0f};

            engine::rotateMatrix_y(m0, -0.76f);
            m0[3] = {-1.0f, 2.0f, 0.0f, 1.0f};

            auto * node3 = scene->createNode();
            node3->setScale(engine::vec3f{0.01f});
            node3->setRotation(engine::vec3f{1.0f, 0.0f, 0.0f});
            node3->setPosition(engine::vec3f(3.5f, 1.0f, 0.0f));

            auto m2 = engine::makeMatrix(0.01F);
            auto * sprite = scene->createObject<engine::scene::Sprite>(textureManager->findFrame("hero_head"), 0u);
            engine::scene::attachObjectToNode(*sprite, *node3);
            sprite->setOrder(1u);

            m2[1][1] *= -1.0f;
            m2[3] = {0.0f, -0.2f, 0.5f, 1.0f};
            auto * node4 = scene->createNode();
            node4->setRotation(engine::vec3f{1.57f, 0.2f, 3.0f});
            node4->setScale(0.015f);
            node4->setPosition(engine::vec3f(0.57f, 0.05f, -0.85f));

            auto * sprite2 = scene->createObject<engine::scene::Sprite>(textureManager->findFrame("hero_sword"), 0u);
            engine::scene::attachObjectToNode(*sprite2, *node4);
            sprite2->setOrder(1u);

            sceneWorldRoot->addChild(node3);

            swordNode = node4;
            sceneWorldRoot->addChild(node4);

            {
                m2[0][0] *= 0.25f;
                m2[1][1] *= -0.25f;

                auto const animId = animationManager.getAnimationIndex("anim0");

                for (size_t i = 0u; i < 10u; ++i) {
                    for (size_t j = 0u; j < 10u; ++j) {
                        auto * node5 = scene->createNode();
                        node5->setScale(engine::vec3f{0.0025f});
                        node5->setPosition(engine::vec3f{-14.0f + static_cast<float>(i) * 0.75f, -1.0f - static_cast<float>(j) * 0.75f, 0.001f});

                        const auto & frame3 = textureManager->findFrame("explosion_33");
                        auto * sprite3 = scene->createObject<engine::scene::Sprite>(frame3, 0u);
                        engine::scene::attachObjectToNode(*sprite3, *node5);
                        sprite3->setOrder(1);
                        sprite3->setEmissive(0.5f);
                        auto * anim0 = scene->createAnimInfo<engine::scene::FramesSequence>(animId, engine::common::random(0.25f, 1.7f));
                        scene->destroyAnimInfo(sprite3->setAnimation(anim0));
                        
                        sceneWorldRoot->addChild(node5);
                    }
                }
            }
            
            { // floor
                auto * node = scene->createNode();
                node->setPosition(engine::vec3f{-30.0f, -30.0f, 0.0f});

                // "assets/vulkan_11_rgba.ktx"
                // "assets/2d_bc7.ktx2"
                // "assets/asphalt-7.jpg"
                // "assets/grass3.png"
                // "assets/images.png"

                const auto * imageHandler = loadImageAsync(*_device, fileManager, *textureManager, "asyncTest", "assets/images.png", 
                    engine::graphics::TextureFlags::eAllowSrgbColorSpace | engine::graphics::TextureFlags::eUseMipMaps);

                const auto frame = engine::graphics::ImageFrame2D{
                    imageHandler, 
                    engine::graphics::Frame2D{.uvMin = {0.0f, 0.0f}, .uvMax = {60.0f, 60.0f}}
                };

                auto * sprite = scene->createObject<engine::scene::Sprite>(frame, 0u);
                sprite->setSize({60.0f, 60.0f});
                engine::scene::attachObjectToNode(*sprite, *node);
                //sprite->setFilter(engine::scene::SpriteTextureFilter::eNearest);
                sprite->setOrder(0);

                sceneWorldRoot->addChild(node);
            }

            { // test

                {
                    auto * node = scene->createNode();
                    node->setPosition(engine::vec3f{17.165f, 12.2f, 0.0f});
                    node->setRotation(engine::vec3f{engine::pi2, 0.0f, -0.35f});

                    const auto * imageHandler = loadImageAsync(*_device, fileManager, *textureManager, "bricks", "assets/154.jpg", 
                        engine::graphics::TextureFlags::eAllowSrgbColorSpace | engine::graphics::TextureFlags::eUseMipMaps);

                    const auto * imageHandler1 = loadImageAsync(*_device, fileManager, *textureManager, "bricks_normal", "assets/154_norm.jpg", 
                        engine::graphics::TextureFlags::eUseMipMaps);

                    const auto frame = engine::graphics::ImageFrame2D{
                        imageHandler, 
                        engine::graphics::Frame2D{.uvMin = {0.0f, 0.0f}, .uvMax = {8.0f, 4.0f}}
                    };

                    auto * sprite = scene->createObject<engine::scene::Sprite>(frame, 7u);
                    sprite->setSubImage(imageHandler1, 0u);
                    sprite->setSubImage(textureManager->getImageHandler("skyBoxCubeMapImage"), 1u, engine::rhi::ImageViewType::eType_Cube);
                    sprite->setSubImage(textureManager->getImageHandler("skyBoxCubeMapImage"), 2u, engine::rhi::ImageViewType::eType_2D_Array);
                    // sprite->setSubImage(textureManager->getDefaultCubeImageHandler(), 1u, engine::rhi::ImageViewType::eType_Cube);
                    //sprite->setFilter(engine::scene::SpriteTextureFilter::eNearest);

                    sprite->setSize({8.3f, 4.0f});
                    engine::scene::attachObjectToNode(*sprite, *node);
                    sprite->setOrder(2);

                    sceneWorldRoot->addChild(node);
                }

                {
                    auto * node = scene->createNode();
                    node->setPosition(engine::vec3f{20.0f, 20.0f, 0.0f});
                    node->setRotation(engine::vec3f{engine::pi2, 0.0f, -0.35f - 1.57f});

                    const auto * imageHandler = loadImageAsync(*_device, fileManager, *textureManager, "bricks", "assets/154.jpg", 
                        engine::graphics::TextureFlags::eAllowSrgbColorSpace | engine::graphics::TextureFlags::eUseMipMaps);

                    const auto * imageHandler1 = loadImageAsync(*_device, fileManager, *textureManager, "bricks_normal", "assets/154_norm.jpg", 
                        engine::graphics::TextureFlags::eUseMipMaps);

                    const auto frame = engine::graphics::ImageFrame2D{
                        imageHandler, 
                        engine::graphics::Frame2D{.uvMin = {0.0f, 0.0f}, .uvMax = {8.0f, 4.0f}}
                    };

                    auto * sprite = scene->createObject<engine::scene::Sprite>(frame, 7u);
                    sprite->setSubImage(imageHandler1, 0u);
                    sprite->setSubImage(textureManager->getImageHandler("skyBoxCubeMapImage"), 1u, engine::rhi::ImageViewType::eType_Cube);
                    sprite->setSubImage(textureManager->getImageHandler("skyBoxCubeMapImage"), 2u, engine::rhi::ImageViewType::eType_2D_Array);
                    // sprite->setSubImage(textureManager->getDefaultCubeImageHandler(), 1u, engine::rhi::ImageViewType::eType_Cube);
                    //sprite->setFilter(engine::scene::SpriteTextureFilter::eNearest);

                    sprite->setSize({8.3f, 4.0f});
                    engine::scene::attachObjectToNode(*sprite, *node);
                    sprite->setOrder(2);

                    sceneWorldRoot->addChild(node);
                }

            }
        }

        //
        shadowMap->prepare();
        const engine::rhi::BufferInfo unifromsInfo {
            engine::rhi::BufferUsageFlags::eUniform_Buffer | engine::rhi::BufferUsageFlags::eTransfer_Dst,
            engine::rhi::MemoryPropertyFlags::eDeviceLocal | engine::rhi::MemoryPropertyFlags::eHostVisible
        };
        shadowMapUniformBuffer = std::make_unique<engine::graphics::InflightBuffer>(
            *_api, *_device, unifromsInfo, sizeof(ShadowMapUniforms)
        );
        shadowMapUniformBuffer->createBuffers();
    }

    void Example::prepare() {
        const auto inflightFrames = _device->inFlightFramesCount();
        _renderQueue = _api->getQueue(*_device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, inflightFrames);

        const uint32_t swapchainImagesCount = _swapchain->getImagesCount();
        _readyToPresentSemaphores.resize(swapchainImagesCount);
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
        drawHelper.createBuffers(*_api, *_device);
        scene->initRenderData(*_api, *_device);

        // on render thread
        _externalCommandLists.resize(inflightFrames);

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
        // need to reset _queryPool, before use it (see on createDefaultResources())
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
            _renderThread->emplaceTask(threads::WorkerTask{[
                transform = camera.getTransform(), 
                view = camera.getView(), 
                projection = camera.getProjection(),
                cameraPos = controller.cameraPosition()
            ](){
                viewParams.cameraView = view;
                viewParams.cameraProjection = projection;
                viewParams.cameraTransform = transform;
                viewParams.cameraPosition = cameraPos; 
                viewParams.nearFarPlanes = kNearFarPlane;
                viewParamsUpdated = true;
            }}, 0u);
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

        static bool showDemo = false;
        if (showDemo) {
            ImGui::ShowDemoWindow();
        }

        if (showOptions) {
            optionsWindow();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("#\t")) {
                if (ImGui::MenuItem("Info")) {
                }
                if (ImGui::MenuItem("Demo")) {
                    showDemo = !showDemo;
                }
                if (ImGui::MenuItem("Options")) {
                    showOptions = !showOptions;
                }
                ImGui::EndMenu();
            }

            ImGui::Text("active threads: %d\t", gStats.activeThreads + 1u); // +1 it's main thread
            ImGui::Text("resolution: %dx%d\t", gStats.width, gStats.height);
            ImGui::Text("frames inflight: %d\t", gStats.inFlightFramesCount);
            static float gDelta = 0.0f;
            static uint16_t gFps = 0u;
            gDelta += delta;
            ++gFps;
            // auto str = engine::fmt_string("fps: %.1f\t", 1'000.0f / gStats.cpuFrameTime);
            auto str = engine::fmt_string("fps: %.1f\t", static_cast<float>(gStats.fps));
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }

            if (gDelta >= 1.0f) {
                gStats.fps = gFps;
                gDelta = 0.0f;
                gFps = 0u;
            }
            str = engine::fmt_string("cpu time: %.3f ms\t", gStats.cpuFrameTime);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }
            str = engine::fmt_string("gpu time: %.3f ms\t", gStats.gpuFrameTime);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }

            str = engine::fmt_string("draw calls: %d\t", gStats.drawCalls);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }

            str = engine::fmt_string("dispatch calls: %d\t", gStats.dispatchCalls);
            if (ImGui::MenuItem(str)) {
                // todo: show graph
            }

            ImGui::EndMainMenuBar();
        }

        _imgui.endFrame();
    }

    void Example::prepareFrameDrawData(engine::rhi::runtime::CommandListPtr & commandList, uint8_t frame, float delta) {

    }

    void Example::draw(threads::ExecutionContext & executionContext, threads::ExecutionTimePoint now, float delta) {
        using namespace engine;
        auto frameTimer = engine::time::Timer{};

        constexpr size_t kGrabbTaskCount = 3u;

        if (!cpuTasks.empty()) {
            auto grabbTaskCount = std::min(kGrabbTaskCount, cpuTasks.size());

            auto tasks = std::vector<BackgroundWorker::CpuJob>{};
            tasks.reserve(grabbTaskCount);

            while (grabbTaskCount > 0u) {
                tasks.emplace_back(std::move(cpuTasks.front()));
                cpuTasks.pop_front();
                --grabbTaskCount;
            }

            backgroundgWorker->execute(std::move(tasks));
        }

        if (!gpuTasks.empty() && _readyToSubmitExternal) {
            _readyToSubmitExternal = false;

            auto grabbTaskCount = std::min(kGrabbTaskCount, gpuTasks.size());

            auto tasks = std::vector<BackgroundWorker::GpuJob>{};
            tasks.reserve(grabbTaskCount);

            while (grabbTaskCount > 0u) {
                tasks.emplace_back(std::move(gpuTasks.front()));
                gpuTasks.pop_front();
                --grabbTaskCount;
            }
            
            backgroundgWorker->submit(std::move(tasks), [this](
                engine::rhi::runtime::CommandList * commandList, std::vector<threads::WorkerTask> && callbacks
            ) mutable {
                _renderThread->emplaceTask(threads::WorkerTask{[
                    this,
                    commandList,
                    callbacks = std::move(callbacks)
                ]() mutable {    
                    _externalCommandLists[_frameIndex()] = commandList;
                    for (auto & callback : callbacks) {
                        callback();
                    }
                    _readyToSubmitExternal = true;
                }}, 1u);
            });
        }

        if (_commandLists.empty()) {
            return;
        }

        const auto [width, height] = _swapchain->getSize();
        const auto frameId = _frameIndex();

        if (_canDraw) {
            auto const waitSubmitResult = _renderQueue->waitSubmit(frameId); // result!!!

            _externalCommandLists[frameId] = nullptr;
            executionContext.performTasks(1u);
        }

        const auto [imageIndex, success] = _swapchain->acquireNextImage(frameId); // https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation

        // printf("frame: %d, imageIndex: %d\n", frameId, imageIndex);

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

        modelManager->generateGpuBuffers(*_api, *_device, *transferCommandList);
        textureManager->generateGpuData(transferCommandList);

        //
        commandList->begin(rhi::CommandBufferUsageFlags::eOne_Time_Submit);

        commandList->resetQueryPool(_queryPool.get(), frameId * 2u, 2u); // must be reset after use and after create too!
        commandList->writeTimestamp(_queryPool.get(), rhi::PipelineStageFlags::eTop_Of_Pipe, frameId * 2u);

        // prepare frame draw data (update uniforms and copy buffers memory)
        prepareFrameDrawData(commandList, frameId, delta);

        if (viewParamsUpdated || lightDirty) { // viewParams has been changed
            shadowMap->setDirection(glm::normalize(lightDir));
            shadowMap->updateCascades(glm::inverse(viewParams.cameraTransform));

            auto * shadowUniforms = shadowMapUniformBuffer->memory2<ShadowMapUniforms>();
            memcpy(shadowUniforms->cascades.data(), shadowMap->getCascadeMatrices().data(), sizeof(engine::mat4f) * shadowMap->getCascadeMatrices().size());
            memcpy(&shadowUniforms->splits, shadowMap->getCascadeSplitDepths().data(), shadowMap->getCascadeSplitDepths().size() * sizeof(float));
            shadowUniforms->options = shadowsOptions;
            shadowMapUniformBuffer->modify();

            viewParamsUpdated = false;
            lightDirty = false;
        }

        shadowMapUniformBuffer->commit(*transferCommandList, frameId);

        // render
        scene->beginFrame(now);

        // scene->fillAttachmentsArray(viewParams, sceneWorldRoot, [](engine::scene::AttachmentObject* object) {
        //     if (object->typeIndex == ctti::typeIndex<engine::scene::AttachmentObject, engine::scene::Mesh>()) {
        //         auto & mesh = *static_cast<engine::scene::Mesh*>(object);
        //         return mesh.getPipeline() == 1u;
        //     }
        //      return false;
        // }, shadowObjects, engine::scene::FillAttachmentFlags::eNone);
 
        shadowObjects.clear();
        shadowObjects.reserve(1024u);

        // auto timer = engine::time::Timer{};
        scene->fillAttachmentsArray(viewParams, sceneWorldRoot, [](engine::scene::AttachmentObject* object) noexcept {
            if (object->typeIndex == ctti::typeIndex<engine::scene::AttachmentObject, engine::scene::Mesh>()) {
                shadowObjects.emplace_back(object); // check visible with shadow matrix?
            }
            return true; 
        }, objects, engine::scene::FillAttachmentFlags::eCalculateQuality);
        // printf("fillAttachmentsArray time: %d\n", timer.elapsed<engine::time::microseconds>());

        engine::scene::Scene::sortObjectsForDraw(shadowObjects);

        scene->animate(now, delta, objects, animationManager, sceneAnimationSpeed); // after fillAttachmentsArrays and after beginFrame for correct

        // auto o0 = std::span<engine::scene::AttachmentObject*>{objects.begin(), objects.begin() + objects.size() / 2};
        // auto o1 = std::span<engine::scene::AttachmentObject*>{objects.begin() + objects.size() / 2, objects.end()};

        //const auto offsets0 = scene->prepareDrawData(shadowObjects, frameId);
        const auto offsets1 = scene->prepareDrawData(objects, frameId);
        
        // const auto offsets0 = scene->prepareDrawData(o0, frameId);
        // auto offsets1 = scene->prepareDrawData(o1, frameId);

        //const auto result0 = scene->dispatch(commandList, frameId, shadowObjects, offsets0);
        const auto result1 = scene->dispatch(commandList, frameId, objects, offsets1);
        //printf("computeSkinVerticesOffset %d\n", result1);
    
        // const auto result = scene->dispatch(commandList, frameId, o0, offsets0);
        // offsets1.computeSkinVerticesOffset = result.computeSkinVerticesOffset;
        // scene->dispatch(commandList, frameId, o1, offsets1);

        shadowMap->beginRenderPass(*commandList, frameId);
        // draw shadows
        // ...
        const auto cascadeMatrices = shadowMap->getCascadeMatrices();
        scene->draw(engine::scene::RenderPass::eShadow, commandList, transferCommandList, frameId, viewParams, shadowObjects, offsets1, &cascadeMatrices);
        shadowMap->endRenderPass(*commandList);

        // draw scene
        // constexpr auto gBufferClearValues = std::array{
        const auto gBufferClearValues = std::array{
            rhi::ClearValue{0x00'00'00'00u},    // position
            rhi::ClearValue{0x00'00'00'00u},    // normal
            rhi::ClearValue{clearColor},        // albedo
            rhi::ClearValue{0x00'00'00'00u},    // emissive
            rhi::ClearValue{0x00'00'00'00u},    // metallicRoughness
            rhi::ClearValue{0.0f, 0u}           // depth
        };

        constexpr auto clearValues = std::array{
            rhi::ClearValue{0x00'00'00'ffu},
        };

        commandList->setViewport(0.0f, 0.0f, width, height);
        commandList->setScissor({{0, 0}, {width, height}});

        // scene pass
        commandList->beginRenderPass(_renderPasses[kGbufferRenderPass], _gBuffers[frameId], gBufferClearValues, {{0, 0}, {width, height}});
        // scene->dispatch(commandList, frameId, objects, offsets);
        
        scene->draw(engine::scene::RenderPass::eCommon, commandList, transferCommandList, frameId, viewParams, objects, offsets1);
        // scene->draw(commandList, transferCommandList, frameId, viewParams, o1, offsets1);
        // scene->draw(commandList, transferCommandList, frameId, viewParams, o0, offsets0);   
        
        if (showBboxes) {
            scene->drawDebug(viewParams, objects);
            drawHelper.prepare(*transferCommandList, frameId);
            drawHelper.draw(*commandList, frameId, now);
        }

        //commandList->nextSubpass(rhi::SubpassContents::ePrimaryBufferInline);
        commandList->endRenderPass();

        rhi::SamplerValue nearestSampler{};
        nearestSampler.info.minFilter = engine::rhi::FilterMode::eNearest;
        nearestSampler.info.magFilter = engine::rhi::FilterMode::eNearest;
        nearestSampler.info.addressModeU = rhi::SamplerAddressMode::eClamp_To_Border;
        nearestSampler.info.addressModeV = rhi::SamplerAddressMode::eClamp_To_Border;
        nearestSampler.info.addressModeW = rhi::SamplerAddressMode::eClamp_To_Border;
        _device->evaluateSampler(nearestSampler);

        rhi::SamplerValue linearSampler{};
        linearSampler.info.minFilter = engine::rhi::FilterMode::eLinear;
        linearSampler.info.magFilter = engine::rhi::FilterMode::eLinear;
        linearSampler.info.addressModeU = rhi::SamplerAddressMode::eClamp_To_Border;
        linearSampler.info.addressModeV = rhi::SamplerAddressMode::eClamp_To_Border;
        linearSampler.info.addressModeW = rhi::SamplerAddressMode::eClamp_To_Border;
        _device->evaluateSampler(linearSampler);

        // ssao pass
        struct SsaoParams {
            engine::mat4f projection;
            //engine::mat4f view;
            engine::vec4f options;
        };

        const SsaoParams ssaoParams {
            .projection = viewParams.cameraProjection,
            //.view = viewParams.cameraView
            .options = {ssaoRadius, ssaoBias, ssaoRangeDepth, static_cast<float>(ssaoRangeMethod)}
        };

        ssao.beginRenderPass(*commandList, frameId);

        commandList->bindPipeline(_pipelines[kSsaoPipeline]);
        commandList->bindVertexBuffer(0u, _postPositions);
        commandList->bindIndexBuffer(_postIndices, 0u, rhi::IndexType::eU16);

        commandList->bindImage(0u, _gBufferAttachments[frameId].position, nearestSampler);
        commandList->bindImage(1u, _gBufferAttachments[frameId].normal, nearestSampler);
        commandList->bindImage(2u, ssao.getNoiseImage(), rhi::SamplerValue{});
        commandList->bindBuffer(3u, ssao.getKernelBuffer());
        commandList->pushConstants(engine::rhi::shaders::Stages::eVertex | engine::rhi::shaders::Stages::eFragment, 0u, sizeof(SsaoParams), &ssaoParams);
        commandList->drawIndexed(6u);

        ssao.endRenderPass(*commandList);

        if (emissiveBlur) { // emissive blur
            engine::vec4f blurUniform{emissiveBlurScale, emissiveBlurStrength, 1.0f, 0.0f};

            bool horizontal = true;
            const auto imagesCount = _gBufferAttachments.size();
            for (uint8_t i = 0u; i < 2u * emissiveBlurSteps; ++i) { // ping - pong
                blurUniform.z = horizontal ? 1.0f : 0.0f;
                commandList->beginRenderPass(_renderPasses[kBlurRenderPass], _emissiveBluredBuffers[frameId + (horizontal ? imagesCount : 0u)], clearValues, {{0, 0}, {width, height}});

                commandList->bindPipeline(_pipelines[kEmissiveBlurPipeline]);
                commandList->bindVertexBuffer(0u, _postPositions);
                commandList->bindIndexBuffer(_postIndices, 0u, rhi::IndexType::eU16);

                commandList->bindImage(0u, horizontal ? _gBufferAttachments[frameId].emissive : _gBufferAttachments[frameId].emissive2, linearSampler);
                //commandList->bindImage(1u, _gBufferAttachments[frameId].position, nearestSampler);
                commandList->pushConstants(engine::rhi::shaders::Stages::eFragment, 0u, sizeof(engine::vec4f), &blurUniform);
                commandList->drawIndexed(6u);

                commandList->endRenderPass();
                horizontal = !horizontal;
            }

        }

        // final pass
        // std::array images2Shaders{rhi::runtime::ImageTransitionInfo{_colorAttachmentImages[imageIndex].get()}};
        commandList->beginRenderPass(_renderPasses[kScreenRenderPass], _mainFrameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}});

        // draw with postEffect
        struct PostParams {
            engine::mat4f view;
            engine::vec4f ambient{};
            engine::vec4f directionalLight{};
            engine::vec4f light{};
            engine::vec4f options{};
        };

        const auto normalMatrix = glm::transpose(glm::inverse(engine::mat3f(viewParams.cameraView)));
        // const PostParams postParams{
        //     .projection = viewParams.cameraProjection,
        //     .cameraPosition = {viewParams.cameraPosition, 0.0f},
        //     //.ambient = ambientColor,
        //     .directionalLight = directionalLightColor,
        //     .light = engine::vec4f{glm::normalize(normalMatrix * light), 0.0f},
        //     .options = {ssaoEnable, static_cast<float>(debugShow), 0.0f, vignetteExtent}
        // };

        PostParams postParams{
            .view = viewParams.cameraView,
            .ambient = ambientColor,
            .directionalLight = directionalLightColor,
            .light = engine::vec4f{glm::normalize(normalMatrix * lightDir), 0.0f},
            .options = {ssaoEnable, static_cast<float>(debugShow), saturation, vignetteExtent}
        };

        const mat4 invProjection = glm::inverse(viewParams.cameraProjection);

        postParams.view[0][3] = invProjection[0][0];
        postParams.view[1][3] = invProjection[1][1];
        postParams.view[3] = {viewParams.cameraPosition, 1.0f};

        commandList->bindPipeline(_pipelines[kPostEffectPipeline]);
        commandList->bindVertexBuffer(0u, _postPositions);
        commandList->bindIndexBuffer(_postIndices, 0u, rhi::IndexType::eU16);
        //commandList->bindImage(0u, _colorAttachmentImages[frameId], rhi::SamplerValue{});

        commandList->bindImage(0u, ssao.getImage(frameId), linearSampler);
        commandList->bindImage(1u, _gBufferAttachments[frameId].normal, nearestSampler);
        commandList->bindImage(2u, _gBufferAttachments[frameId].albedo, nearestSampler);
        commandList->bindImage(3u, _gBufferAttachments[frameId].emissive, linearSampler);
        commandList->bindImage(4u, _gBufferAttachments[frameId].position, nearestSampler);
        commandList->bindImage(5u, _gBufferAttachments[frameId].metallicRoughness, linearSampler);
        commandList->bindImage(
            6u, shadowMap->getImage(frameId), shadowMap->getSampler(), 0u, 
            rhi::ImageAspectFlags::eDepth, rhi::ImageViewType::eType_2D_Array 
        );
        commandList->bindBuffer(7u, (*shadowMapUniformBuffer)(frameId), 0u, 0u);

        // commandList->bindImage(
        //     0u, skyBoxCubeMapImage, linearSampler, 1u, 
        //     rhi::ImageAspectFlags::eColor, rhi::ImageViewType::eType_Cube 
        // );

        commandList->pushConstants(rhi::shaders::Stages::eVertex | rhi::shaders::Stages::eFragment, 0u, sizeof(PostParams), &postParams);
        commandList->drawIndexed(6u);

        // draw imgui
        _imgui.prepare(*transferCommandList, frameId, *_api, *_device);
        _imgui.draw(*commandList, frameId);

        commandList->endRenderPass();

        scene->endFrame(commandList, transferCommandList, frameId);

        commandList->writeTimestamp(_queryPool.get(), rhi::PipelineStageFlags::eBottom_Of_Pipe, frameId * 2u + 1u);
        commandList->end();
        transferCommandList->end();

        transferCommandList->record(frameId);
        commandList->record(frameId);
        // render frame

        // query pool results
        auto results = std::span{reinterpret_cast<std::byte*>(_queryResults[frameId].data()), 
            _queryResults[frameId].size() * sizeof(uint64_t)};

        _queryPool->getResults(frameId * 2u, 2u, results, 2u * sizeof(uint64_t), 
            rhi::QueryResultFlags::e64 | rhi::QueryResultFlags::eWith_Availability);

        if (_queryResults[frameId][3u] != 0u) {
            const float gpuFrameTime = _gpu->timestampToMilliseconds(_queryResults[frameId][2u] - _queryResults[frameId][0u]);    
            gStats.gpuFrameTime = gpuFrameTime;
            //printf("_queryResults[2] %llu\n", _queryResults[frameId][2]);
            //printf("_queryResults[0] %llu\n", _queryResults[frameId][0]);
        } else {
            printf("_queryResults no available!\n");
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
        const auto signalSemaphores = std::array{_readyToPresentSemaphores[imageIndex]};
        const auto waitStages = std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output};

        auto transferLists = std::array{transferCommandList.get()};
        auto lists = std::array{commandList.get()};

        if (_externalCommandLists[frameId]) {
            auto extLists = std::array{_externalCommandLists[frameId]};

            bool const submitted = _renderQueue->submit(frameId, std::array{
                engine::rhi::runtime::Queue::SubmitInfo {
                    .w8Semaphores = {},
                    .w8DstStages = {},
                    .signalSemaphores = {},
                    .lists = extLists
                },
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
        } else {
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
        }

        ++_frameIndex;

        // present
        auto const presentResult = _renderQueue->present(_swapchain.get(), imageIndex, signalSemaphores);

        gStats.drawCalls = 0u;
        gStats.dispatchCalls = 0u;
        gStats.cpuFrameTime = static_cast<float>(frameTimer.elapsed<engine::time::microseconds>()) * 0.001f;
        gStats.drawCalls = commandList->getStats().drawCalls;
        gStats.drawCalls += transferCommandList->getStats().drawCalls;
        gStats.dispatchCalls = commandList->getStats().dispatchCalls;
        gStats.dispatchCalls += transferCommandList->getStats().dispatchCalls;
    }

}