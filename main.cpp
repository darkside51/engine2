#include "utils/profile/leaks.h"
#include "threads/executor.h"
#include "threads/worker.h"
#include "threads/utils.h"

#include "threads/thread_pool/thread_pool.h"

#include "utils/profile/time.h"
#include "common/version.h"

#include <cstdio>
#include "tests/all_tests.h"

//
#include "device/glfw/device.h"
#include "log/log.h"
#include "file/file_manager.h"
#include "file/common_file_system.h"

//
#include "graphics/render/rhi/common/api.h"
#include "graphics/render/rhi/common/utils.h"
#include "graphics/render/rhi/common/pipeline.h"
#include "graphics/render/rhi/common/commands.h"
#include "graphics/render/rhi/common/command_list.h"

#include "graphics/render/rhi/vulkan/runtime.h"

#include "graphics/render/inflight_buffer.h"

#include "math/math.h"

#include <algorithm>

engine::rhi::runtime::BufferPtr stagePositions = nullptr;
engine::rhi::runtime::BufferPtr stageColors = nullptr;
engine::rhi::runtime::BufferPtr positions = nullptr;
engine::rhi::runtime::BufferPtr colors = nullptr;
std::unique_ptr<engine::graphics::InflightBuffer> uniformBufferI = nullptr;
engine::rhi::runtime::BufferPtr uniformBuffer2 = nullptr;

engine::rhi::RenderPassDescription createRenderPassDescription(engine::rhi::Format colorFormat, engine::rhi::Format depthFormat) {
    std::vector<engine::rhi::AttachmentDescription> attachments(2u);
    attachments[0].format = colorFormat;
    attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
    attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[0].finalLayout = engine::rhi::ImageLayout::ePresent_Src;

    attachments[1].format = depthFormat;
    attachments[1].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eDont_Care};
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
        {} // std::vector<engine::rhi::SubpassDependency>
    };
}

auto loadShaders(const std::unique_ptr<engine::rhi::runtime::Rhi> & api, 
    std::unique_ptr<engine::rhi::runtime::Device> & device, 
    std::unique_ptr<engine::rhi::runtime::ShaderModulesManager> & shaderModulesManager) {

    engine::FileManager fm;
    auto * fsystem = fm.createFileSystem<engine::CommonFileSystem>("common_fs0", engine::CommonFileSystem::getCurrentDirectory() + "./resources/");

    auto const vBytes = fm.readFile("shaders/simple.vert.spv");
    auto const fBytes = fm.readFile("shaders/simple.frag.spv");

    const auto & vsModule = shaderModulesManager->registerModule("simple.vert", *device, engine::rhi::shaders::Stages::eVertex, vBytes); // call overloaded function
    const auto & fsModule = shaderModulesManager->registerModule("simple.frag", *device, engine::rhi::shaders::Stages::eFragment, fBytes);

    return api->createProgram(device, std::array{vsModule.getView(), fsModule.getView()}, {.dynamicDescriptorsMask = 0b10u});
}

auto createPipeline(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device,
    engine::rhi::runtime::RenderPass renderPass,
    const std::unique_ptr<engine::rhi::runtime::GpuProgram> & program) {
    using namespace engine::rhi;

    GraphicsPipelineStatesInfo info;
    info.viewportState = {1u, 1u};
    info.blendModes = {common_blend_modes::blend_none};
    info.depthStencilState = {
        {
            .depthTestEnable = true,
            .depthWriteEnable = true,
            .depthCompareOp = CompareOp::eLess
        }, 
        {}
    };
    info.dynamicStates = DynamicState::eViewport | DynamicState::eScissor;
    info.rasterisationState = {PolygonMode::eFill, CullMode::eBack};
    info.inputAssemblyState = {PrimitiveTopology::eTriangles, false};
    info.vertexInputState = {
        std::vector{VertexInputBinding::create<float[3]>(0u), VertexInputBinding::create<uint32_t>(1u)},
        std::vector{
            VertexInputAttribute{
                .location = 0u,
                .binding = 0u,
                .format = Format::eR32G32B32_Sfloat,
                .offset = 0u
            },
            VertexInputAttribute{
                .location = 1u,
                .binding = 1u,
                .format = Format::eR8G8B8A8_Unorm,
                .offset = 0u
            }
        }
    };

   return api->createGraphicsPipeline(device, renderPass, program, info);
}

void createBuffers(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device) {
    using namespace engine::rhi;

    const engine::rhi::BufferInfo stageInfo {
        engine::rhi::BufferUsageFlags::eTransfer_Src,
        MemoryPropertyFlags::eHostVisible
    };

    stagePositions = api->createBuffer(device, stageInfo, sizeof(std::array<float, 3>) * 3u);
    {
        auto * map = stagePositions->map2<std::array<float, 3>>();
        map[0u] = {-0.5f, 0.5f, 0.1f};
        map[1u] = {0.5f, 0.5f, 0.1f};
        map[2u] = {0.0f, -0.5f, 0.1f};
        stagePositions->flush();
        stagePositions->unmap();
    }

    stageColors = api->createBuffer(device, stageInfo, sizeof(uint32_t) * 3u);
    {
        auto * map = stageColors->map2<uint32_t>();
        map[0u] = 0xff'00'ff'00u; map[1u] = 0xff'00'00'ffu; map[2u] = 0xff'ff'00'00u;
        stageColors->flush();
        stageColors->unmap();
    }

    const engine::rhi::BufferInfo localInfo {
        engine::rhi::BufferUsageFlags::eVertex_Buffer | engine::rhi::BufferUsageFlags::eTransfer_Dst,
        engine::rhi::MemoryPropertyFlags::eDeviceLocal
    };

    positions = api->createBuffer(device, localInfo, stagePositions->sizeInBytes());
    colors = api->createBuffer(device, localInfo, stageColors->sizeInBytes());

    // uniforms
    engine::rhi::BufferInfo unifromsInfo {
        engine::rhi::BufferUsageFlags::eUniform_Buffer,
        engine::rhi::MemoryPropertyFlags::eHostVisible
    };

    uniformBuffer2 = api->createBuffer(device, unifromsInfo, sizeof(engine::vec2f));
    {
        auto * map = uniformBuffer2->map2<engine::vec2f>();
        map[0u] = {0.0f, 0.25f};
        uniformBuffer2->flush();
        uniformBuffer2->unmap();
    }

    unifromsInfo.memoryFlags = engine::rhi::MemoryPropertyFlags::eDeviceLocal;
    unifromsInfo.usage = unifromsInfo.usage | engine::rhi::BufferUsageFlags::eTransfer_Dst;

    const size_t kUniformBufferOffsetAlign = device->getUniformBufferOffsetAlignment();
    uniformBufferI = std::make_unique<engine::graphics::InflightBuffer>(api, device, unifromsInfo, std::max(kUniformBufferOffsetAlign, sizeof(engine::vec2f)) * 2u);
    {
        auto * map = uniformBufferI->map2<engine::vec2f>();
        map[0u] = {-0.5f, 0.0};
        map[kUniformBufferOffsetAlign / sizeof(engine::vec2f) + 0u] = {0.0f, -0.25f};
        uniformBufferI->unmap();
    }
}

int main() {
    // engine::tests::execute_tests();

    // todo: 
    // 1. impl staging buffer for inflightBuffer - in progress
    // 2. textures (as command_list resources like a buffers) (may be framebuffers as command_list resources?)
    // 3. imgui
    // 4. simplify
    // 5. optimize
    // 6. resize
    // 7. input
    // 8. rhi::Result
    // 9. refactoring

    engine::GLFWModule glfwModule;
    
    const engine::rhi::Api<engine::vulkan::Api> api;
    using Api = decltype(api);

    auto & window0 = glfwModule.createWindow(Api::name().data());
    window0.setVisible(false);

    bool const kUseApiDebugMessages = true;

    engine::rhi::InstanceCreateInfo instanceCreateInfo {
        .apiVersion = engine::common::Version{1, 3, 0},
        .extensions = {"VK_KHR_surface", "VK_KHR_win32_surface", "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
		    "VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface", 
            Api::Impl::kDebugUtilExtName // for validation
            },
        .layers = {Api::Impl::kLayerValidationKHR}, // for validation
        .createDebugMessages = kUseApiDebugMessages,
        .allocationCallback = nullptr
    };

    std::unique_ptr<engine::rhi::runtime::Rhi> rApi = std::make_unique<engine::vulkan::runtime::Rhi>(instanceCreateInfo);
    
    engine::rhi::PhysicalDeviceFeatureRequest gpuFeatures{};
    auto gpuDevices = rApi->enumerateDevices({gpuFeatures, {engine::vulkan::runtime::Rhi::Api::Impl::kSwapchainExtName}});

    const auto surface = rApi->createSurface(window0.getSurfaceCreateInfo());

    if (auto & gpuDevice = gpuDevices[0u]; gpuDevice->checkSurfaceSupport(*surface)) {
        const auto queueFlags = engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present;
        const auto [index, family] = gpuDevice->getQueueFamily(queueFlags);

        engine::rhi::DeviceCreateInfo deviceCreateInfo{gpuFeatures};
        deviceCreateInfo.queueFamilies.push_back({index, 1u, {1.0f}, queueFlags});
        deviceCreateInfo.commandPoolThreadsCount = 1u;
        deviceCreateInfo.inFlightFramesCount = 3u;

        auto device = rApi->createDevice(gpuDevice, deviceCreateInfo);
        const auto depthFormat = device->getSupportedDepthFormat(32u, false);
        const uint8_t inflightFrames = device->inFlightFramesCount();

        engine::rhi::SwapchainCreateInfo swapchainCreateInfo = {};
        swapchainCreateInfo.imagesCount = inflightFrames;
        swapchainCreateInfo.queueFamilies.push_back(index);

        auto swapchain = rApi->createSwapchain(device, surface, swapchainCreateInfo);

        auto const [width, height] = window0.getSize();
        swapchain->resize(width, height, false);
        const uint32_t swapchainImagesCount = swapchain->getImagesCount();

        auto renderPass = rApi->getRenderPass(device, createRenderPassDescription(swapchain->getFormat(), depthFormat));
        
        auto imageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment,
            depthFormat, {width, height, 1u}
        }; 
        imageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        std::vector<std::unique_ptr<engine::rhi::runtime::Image>> depthImages;
        depthImages.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            depthImages.emplace_back(std::move(rApi->createImage(device, imageInfo)));
        }

        std::vector<std::unique_ptr<engine::rhi::runtime::FrameBuffer>> frameBuffers;
        frameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            auto & image = swapchain->getImage(i);
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};
            fbAttachemnts[0u].image = &image;
            fbAttachemnts[0u].imageSize = {image.getExtent().width, image.getExtent().height};

            auto & depthImage = depthImages[i];
            fbAttachemnts[1u].image = depthImage.get();
            fbAttachemnts[1u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
            fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?

            frameBuffers.emplace_back(rApi->createFrameBuffer(device, std::move(fbAttachemnts), renderPass));
        }

        auto queue = rApi->getQueue(device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, inflightFrames);

        std::vector<engine::rhi::runtime::Semaphore> readyToPresentSemaphores(inflightFrames);
        for (auto & semaphore : readyToPresentSemaphores) {
            semaphore = rApi->createSemaphore(device);
        }

        auto shaderModulesManager = rApi->createShaderModulesManager();
        auto program = loadShaders(rApi, device, shaderModulesManager);
        auto pipeline = createPipeline(rApi, device, renderPass, program);
        createBuffers(rApi, device);

        std::vector<engine::rhi::runtime::CommandListPtr> commandLists;
        
        constexpr float inputCheckTime = 1.0f / 60.0f;
        constexpr float redrawTime = 1.0f / 120.0f;

        constexpr auto clearValues = std::array{
            engine::rhi::ClearValue{0x77'77'77'ffu},
            engine::rhi::ClearValue{1.0f}
        };

        engine::rhi::FrameIndex frameIndex{inflightFrames};

        const size_t kUniformBufferOffsetAlign = device->getUniformBufferOffsetAlignment();

        threads::Executor w([&](auto & executionContext, float delta) {
            executionContext.performTasks();
            if (!commandLists.empty()) {
                const auto frameId = frameIndex();

                auto const waitSubmitResult = queue->waitSubmit(frameId); // result!!!

                const auto [imageIndex, success] = swapchain->acquireNextImage(frameId);

                if (!success) {
                    return;
                }
                device->resetFrameTmpData(frameId);

                // render frame
                auto & commandList = commandLists[frameId];
                commandList->begin();

                if (stagePositions) {
                    commandList->copyBuffer2Buffer(stagePositions, positions, 
                        {.srcOffset = 0u, .dstOffset = 0u, .size = stagePositions->sizeInBytes()});

                    commandList->copyBuffer2Buffer(stageColors, colors, 
                        {.srcOffset = 0u, .dstOffset = 0u, .size = stageColors->sizeInBytes()});

                    stagePositions = nullptr;
                    stageColors = nullptr;
                }

                { // update uniforms
                    auto * map = uniformBufferI->map2<engine::vec2f>();

                    if (map[0u].x > 1.5f) { map[0u].x = -1.5f; }
                    map[0u].x += 0.5f * delta;
      
                    uniformBufferI->unmap();
                    uniformBufferI->commit(commandList, frameId);
                }

                commandList->beginRenderPass(renderPass, frameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}});

                commandList->setViewport(0.0f, 0.0f, width, height);
                commandList->setScissor({{0, 0}, {width, height}});

                commandList->bindPipeline(pipeline);

                // first triangle
                commandList->bindBuffer(0u, (*uniformBufferI)(frameId), 0u, 
                    0u, sizeof(engine::vec2f));
                commandList->bindBuffer(1u, (*uniformBufferI)(frameId), 0u, 
                    engine::align_value(sizeof(engine::vec2f), kUniformBufferOffsetAlign), sizeof(engine::vec2f));

                commandList->bindVertexBuffer(0u, positions);
                commandList->bindVertexBuffer(1u, colors);
                commandList->draw(3u);

                // second triangle
                // commandList->bindBuffer(1u, uniformBuffer2);
                commandList->bindBuffer(1u, (*uniformBufferI)(frameId), 0u, 0u, sizeof(float) * 2u);
                commandList->draw(3u);
                
                commandList->endRenderPass();
                commandList->end();

                commandList->record(frameId);
                // render frame

                // submit
                const auto waitSemaphores = std::array{swapchain->imageAvailableSemaphore(frameId)};
                const auto signalSemaphores = std::array{readyToPresentSemaphores[frameId]};
                const auto waitStages = std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output};
                auto lists = std::array{commandLists[frameId].get()};

                bool const submitted = queue->submit(frameId, std::array{
                    engine::rhi::runtime::Queue::SubmitInfo {
                        .w8Semaphores = waitSemaphores,
                        .w8DstStages = waitStages,
                        .signalSemaphores = signalSemaphores,
                        .lists = lists
                    }
                });

                ++frameIndex;

                // present
                auto const presentResult = queue->present(swapchain.get(), imageIndex, signalSemaphores);
            }
        }, redrawTime);

        w.run();

        w.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            auto * commandPool = device->acquireCommandPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
            commandLists.reserve(inflightFrames);
            for (size_t i = 0u; i < inflightFrames; ++i) {
                commandLists.emplace_back(rApi->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary));
            }
        }}));

        window0.setVisible(true);

        window0.setOnClose([&]() {
            w.stop();
            device->waitIdle();

            stagePositions = nullptr;
            stageColors = nullptr;

            positions = nullptr;
            colors = nullptr;

            uniformBufferI = nullptr;
            uniformBuffer2 = nullptr;

            for (auto & semaphore : readyToPresentSemaphores) {
                rApi->destroySemaphore(device, semaphore);
            }
        });

        while (glfwModule.process()) {
            std::this_thread::sleep_for(std::chrono::duration<float>(inputCheckTime));
        }
    }

    return 1;
}