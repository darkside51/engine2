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
#include "graphics/render/rhi/common/commands.h"
#include "graphics/render/rhi/common/command_list.h"

#include "graphics/render/rhi/vulkan/runtime.h"
#include "graphics/render/rhi/vulkan/device.h"
#include "graphics/render/rhi/vulkan/queue.h"

engine::rhi::RenderPassDescription createRenderPassDescription(engine::rhi::Format colorFormat, engine::rhi::Format depthFormat) {
    std::vector<engine::rhi::AttachmentDescription> attachments(2u);
    attachments[0].format = colorFormat;
    attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
    attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[0].initialLayout = engine::rhi::ImageLayout::eUndefined;
    attachments[0].finalLayout = engine::rhi::ImageLayout::ePresent_Src;

    attachments[1].format = depthFormat;
    attachments[1].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[1].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[1].initialLayout = engine::rhi::ImageLayout::eUndefined;
    attachments[1].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;

    std::vector<engine::rhi::SubpassDescription> subpasses{1};
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

auto loadShadersRuntime(const std::unique_ptr<engine::rhi::runtime::Rhi> & api, 
    std::unique_ptr<engine::rhi::runtime::Device> & device, 
    std::unique_ptr<engine::rhi::runtime::ShaderModulesManager> & shaderModulesManager) {

    engine::FileManager fm;
    auto * fsystem = fm.createFileSystem<engine::CommonFileSystem>("main", engine::CommonFileSystem::getCurrentDirectory() + "./resources/");

    auto const vBytes = fm.readFile("shaders/simple.vert.spv");
    auto const fBytes = fm.readFile("shaders/simple.frag.spv");

    const auto & vsModule = shaderModulesManager->registerModule("simple.vert", *device, engine::rhi::shaders::Stages::eVertex, vBytes); // call overloaded function
    const auto & fsModule = shaderModulesManager->registerModule("simple.frag", *device, engine::rhi::shaders::Stages::eFragment, fBytes);

    return api->createProgram(device, std::array{vsModule.getView(), fsModule.getView()});
}

auto createPipelineRuntime(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device,
    engine::rhi::runtime::RenderPass renderPass,
    const std::unique_ptr<engine::rhi::runtime::GpuProgram> & program) {
    using namespace engine::rhi;

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
    info.dynamicStates = DynamicState::eViewport | DynamicState::eScissor;
    info.rasterisationState = {PolygonMode::eFill, CullMode::eBack};
    info.inputAssemblyState = {PrimitiveTopology::eTriangles, false};
    info.vertexInputState = {
        std::vector{VertexInputBinding::create<float[3]>(0u)},
        std::vector{
            VertexInputAttribute{
                .location = 0u,
                .binding = 0u,
                .format = Format::eR32G32B32_Sfloat,
                .offset = 0u
            }
        }
    };

   return api->createGraphicsPipeline(device, renderPass, program, info);
}

auto createBuffersRuntime(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device) {
    using namespace engine::rhi;

    const engine::rhi::BufferInfo info {
        BufferUsageFlags::eVertex_Buffer,
        MemoryPropertyFlags::eHostVisible
    };

    auto resultBuffer = api->createBuffer(device, info, sizeof(float) * 9u);
    {
        auto * map = resultBuffer->map2<float>();
        map[0u] = -0.5f; map[1u] = 0.5f; map[2u] = 0.1f;
        map[3u] = 0.5f; map[4u] = 0.5f; map[5u] = 0.1f;
        map[6u] = 0.0f; map[7u] = -0.5f; map[8u] = 0.1f;
        resultBuffer->flush();
        resultBuffer->unmap();
    }

    return resultBuffer;
}

int main() {
    engine::tests::execute_tests();

    const engine::rhi::Api<engine::vulkan::Api> api;
    using Api = decltype(api);

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

    engine::GLFWModule glfwModule;
    auto & window0 = glfwModule.open(Api::name().data());

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
        deviceCreateInfo.bufferization = 3u;

        auto device = rApi->createDevice(gpuDevice, deviceCreateInfo);
        const auto depthFormat = device->getSupportedDepthFormat(32u, false);
        const uint8_t buffersCount = device->bufferization();

        engine::rhi::SwapchainCreateInfo swapchainCreateInfo = {};
        swapchainCreateInfo.imagesCount = buffersCount;
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

        engine::rhi::FrameIndex frameIndex{static_cast<uint8_t>(buffersCount)};

        auto queue = rApi->getQueue(device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, buffersCount);

        std::vector<engine::rhi::runtime::Semaphore> readyToPresentSemaphores(buffersCount);
        for (auto & semaphore : readyToPresentSemaphores) {
            semaphore = rApi->createSemaphore(device);
        }

        auto shaderModulesManager = rApi->createShaderModulesManager();
        auto program = loadShadersRuntime(rApi, device, shaderModulesManager);
        auto pipeline = createPipelineRuntime(rApi, device, renderPass, program);
        auto buffer = createBuffersRuntime(rApi, device);

        const engine::rhi::BufferInfo info {
            engine::rhi::BufferUsageFlags::eUniform_Buffer,
            engine::rhi::MemoryPropertyFlags::eHostVisible
        };
    
        auto uniformBuffer = rApi->createBuffer(device, info, sizeof(float) * 2u);
        {
            auto * map = uniformBuffer->map2<float>();
            map[0u] = -0.5f;
            map[1u] = 0.0f;
            uniformBuffer->flush();
            uniformBuffer->unmap();
        }

        auto uniformBuffer2 = rApi->createBuffer(device, info, sizeof(float) * 2u);
        {
            auto * map = uniformBuffer2->map2<float>();
            map[0u] = 0.0f;
            map[1u] = 0.25f;
            uniformBuffer2->flush();
            uniformBuffer2->unmap();
        }

        // reserve frames data
        std::vector<engine::rhi::runtime::CommandListPtr> commandLists;
        
        bool renderInited = false;
        constexpr float inputCheckTime = 1.0f / 60.0f;
        constexpr float redrawTime = 1.0f / 120.0f;

        constexpr auto clearValues = std::array{
            engine::rhi::ClearValue{0x77'77'77'ffu},
            engine::rhi::ClearValue{1.0f}
        };

        threads::Executor w([&](auto & executionContext, float delta) {
            executionContext.performTasks();
            if (renderInited) {
                const auto frameId = frameIndex();

                auto const waitSubmitResult = queue->waitSubmit(frameId); // result!!!

                //
                { // test ubo update
                    auto * map = uniformBuffer->map2<float>();
                    map[0u] += 0.1f * delta;
                    map[1u] = 0.0f;
                    uniformBuffer->flush();
                    uniformBuffer->unmap();
                }
                //

                const auto [imageIndex, success] = swapchain->acquireNextImage(frameId);

                if (!success) {
                    return;
                }
                device->resetFrameTmpData(frameId);

                // render frame
                auto & commandList = commandLists[frameId];
                commandList->begin();

                commandList->beginRenderPass(renderPass, frameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}});

                commandList->setViewport(0.0f, 0.0f, width, height);
                commandList->setScissor({{0, 0}, {width, height}});

                commandList->bindPipeline(pipeline);

                commandList->bindBuffer(0u, uniformBuffer);
                commandList->bindBuffer(1u, uniformBuffer2);

                commandList->bindVertexBuffer(0u, buffer.get());
                commandList->draw(3u);
                
                commandList->endRenderPass();
                commandList->end();

                commandList->flush(frameId);
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
            commandLists.reserve(buffersCount);
            for (size_t i = 0; i < buffersCount; ++i) {
                commandLists.emplace_back(rApi->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary));
            }
            renderInited = true;
        }}));

        window0.setOnClose([&]() {
            w.stop();
            device->waitIdle();

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