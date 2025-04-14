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
#include "graphics/render/rhi/vulkan/api.h"
#include "graphics/render/rhi/vulkan/vk_types.h"
#include "graphics/render/rhi/vulkan/instance.h"
#include "graphics/render/rhi/vulkan/physical_device.h"
#include "graphics/render/rhi/vulkan/surface.h"
#include "graphics/render/rhi/vulkan/device.h"
#include "graphics/render/rhi/vulkan/swapchain.h"
#include "graphics/render/rhi/vulkan/buffer.h"
#include "graphics/render/rhi/vulkan/image.h"
#include "graphics/render/rhi/vulkan/renderpass.h"
#include "graphics/render/rhi/vulkan/framebuffer.h"
#include "graphics/render/rhi/vulkan/command_pool.h"
#include "graphics/render/rhi/vulkan/command_buffer.h"
#include "graphics/render/rhi/vulkan/queue.h"
#include "graphics/render/rhi/vulkan/utils.h"
#include "graphics/render/rhi/vulkan/descriptor_pool_manager.h"
#include "graphics/render/rhi/vulkan/descriptor_set_updater.h"
#include "graphics/render/rhi/vulkan/gpu_program.h"
#include "graphics/render/rhi/vulkan/pipeline.h"

using gApi = engine::rhi::Api<engine::vulkan::Api>;

engine::rhi::RenderPassDescription createRenderPassDescription(engine::rhi::Format colorFormat, engine::rhi::Format depthFormat) {
    std::vector<engine::rhi::AttachmentDescription> attachments(2u);
    attachments[0].format = colorFormat;
    attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
    attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[0].initialLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;
    attachments[0].finalLayout = engine::rhi::ImageLayout::ePresent_Src;

    attachments[1].format = depthFormat;
    attachments[1].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[1].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[1].initialLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;
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

auto createDescriptorSets(auto & device, const engine::rhi::shaders::BindingSetLayout & setLayout) {
    using namespace engine::rhi;
    using Api = std::decay_t<decltype(device)>::Api;

    auto & descriptorPoolManager = device.getDescriptorPoolManager();

    // auto & descriptorPool = descriptorPoolManager.createPool(
    //     std::array{
    //         std::pair{shaders::DescriptorType::eUniform_Buffer, 10u},
    //         std::pair{shaders::DescriptorType::eStorage_Buffer, 10u},
    //         std::pair{shaders::DescriptorType::eCombined_Image_Sampler, 10u}
    //     }
    // );

    auto & pool = descriptorPoolManager.getCompatiblePool(setLayout);
    return pool.allocateDescriptorSet(setLayout);
}

engine::vulkan::ShaderModule vsModule0;
engine::vulkan::ShaderModule fsModule0;
gApi::GpuProgram *gProgram0;
gApi::Buffer *vtxBuffer;
gApi::Pipeline *pipeline = nullptr;

auto createPipeline(gApi::Device & device, VkRenderPass renderPass) {
    using namespace engine;
    using Api = gApi;

    Api::GraphicsPipelineInfo info{};
    info.viewportStateInfo(1u, 1u)
    .dynamicStateInfo(rhi::DynamicState::eViewport | rhi::DynamicState::eScissor)
    .multisampleStateInfo()
    .rasterizationStateInfo(rhi::PolygonMode::eFill, rhi::CullMode::eBack)
    .inputAssemblyStateInfo(rhi::PrimitiveTopology::eTriangles)
    .depthStencilStateInfo({
            .depthTestEnable = false,
            .depthWriteEnable = false,
            .depthCompareOp = rhi::CompareOp::eAlways
        }, {})
    .colorBlendStateCreateInfo(std::array{rhi::common_blend_modes::blend_none})
    .vertexInputStateInfo(
        std::array{rhi::VertexInputBinding::create<float[3]>(0u)},
        std::array{rhi::VertexInputAttribute{
                .location = 0u,
                .binding = 0u,
                .format = rhi::Format::eR32G32B32_Sfloat,
                .offset = 0u
            }
        }
    )
    .tessellationStateInfo(0u);

    return typename Api::Pipeline{device, info.create(*gProgram0, renderPass)};
}

auto loadShaders(auto & device) {
    using namespace engine::rhi;
    using Api = std::decay_t<decltype(device)>::Api;

    engine::FileManager fm;
    auto * fsystem = fm.createFileSystem<engine::CommonFileSystem>("main", engine::CommonFileSystem::getCurrentDirectory() + "./resources/");

    auto const vBytes = fm.readFile("shaders/simple.vert.spv");
    auto const fBytes = fm.readFile("shaders/simple.frag.spv");

    const engine::vulkan::ShaderModule vsModule{device, engine::rhi::shaders::Stages::eVertex, vBytes};
    const engine::vulkan::ShaderModule fsModule{device, engine::rhi::shaders::Stages::eFragment, fBytes};

    return typename Api::GpuProgram{device, std::array{vsModule.view(), fsModule.view()}};
}

auto createBuffers(auto & device) {
    using namespace engine::rhi;
    using Api = std::decay_t<decltype(device)>::Api;

    engine::rhi::BufferInfo info{
        .usage = BufferUsageFlags::eVertex_Buffer,
        .memoryFlags = MemoryPropertyFlags::eHostVisible,
        .memoryExtFlags = MemoryExtendedFlags::eNone,
        .flags = BufferCreateFlags::eNone,
        .sharing = SharingMode::eExclusive,
        .queueFamilyIndices = {0}
    };

    typename Api::Buffer buffer{device, info, sizeof(float) * 4u};
    {
        auto * map = buffer.template map2<float>();
        map[0u] = 10.0f;
        map[1u] = 20.0f;
        map[2u] = 30.0f;
        map[3u] = 40.0f;
        buffer.flush();
        buffer.unmap();
    }

    auto name = buffer.getName();
    buffer.setName("buffer0");
    name = buffer.getName();

    ///////////////////
    shaders::BindingSetLayout setLayout {
        {
            {
                .binding = 0u,
                .type = shaders::DescriptorType::eUniform_Buffer,
                .shaderStages = shaders::Stages::eVertex,
                .count = 1u
            },
            {
                .binding = 1u,
                .type = shaders::DescriptorType::eStorage_Buffer,
                .shaderStages = shaders::Stages::eVertex,
                .count = 1u
            }
        }
    };

    auto descriptorSet = createDescriptorSets(device, setLayout);

    engine::rhi::BufferInfo uniformInfo{
        .usage = BufferUsageFlags::eUniform_Buffer,
        .memoryFlags = MemoryPropertyFlags::eHostVisible,
        .memoryExtFlags = MemoryExtendedFlags::eNone,
        .flags = BufferCreateFlags::eNone,
        .sharing = SharingMode::eExclusive,
        .queueFamilyIndices = {0}
    };

    engine::rhi::BufferInfo storageInfo{
        .usage = BufferUsageFlags::eStorage_Buffer,
        .memoryFlags = MemoryPropertyFlags::eHostVisible,
        .memoryExtFlags = MemoryExtendedFlags::eNone,
        .flags = BufferCreateFlags::eNone,
        .sharing = SharingMode::eExclusive,
        .queueFamilyIndices = {0}
    };

    typename Api::Buffer uniformBuffer{device, uniformInfo, sizeof(float) * 2u};
    {
        auto * map = uniformBuffer.template map2<float>();
        map[0u] = 10.0f;
        map[1u] = 20.0f;
        uniformBuffer.flush();
        uniformBuffer.unmap();
    }

    typename Api::Buffer storageBuffer{device, storageInfo, sizeof(float) * 2u};
    {
        auto * map = storageBuffer.template map2<float>();
        map[0u] = 10.0f;
        map[1u] = 20.0f;
        storageBuffer.flush();
        storageBuffer.unmap();
    }
    
    engine::vulkan::DescriptorSetUpdater updater{device, setLayout};
    // updater.updateBuffers(std::array{std::pair{&uniformBuffer, 0u}, std::pair{&storageBuffer, 1u}});
    updater.updateBuffers(std::array{std::pair{&uniformBuffer, 0u}});
    updater.updateBuffers(std::array{std::pair{&storageBuffer, 1u}});
    updater.flush(descriptorSet);

    ///////////////////////////
    auto resultBuffer = typename Api::Buffer{device, info, sizeof(float) * 9u};
    {
        auto * map = resultBuffer.template map2<float>();
        map[0u] = -0.5f; map[1u] = 0.5f; map[2u] = 0.1f;
        map[3u] = 0.5f; map[4u] = 0.5f; map[5u] = 0.1f;
        map[6u] = 0.0f; map[7u] = -0.5f; map[8u] = 0.1f;
        resultBuffer.flush();
        resultBuffer.unmap();
    }

    return resultBuffer;
}

int main() {
    engine::tests::execute_tests();

    // std::variant<std::monostate, engine::vulkan::Api, int> api2;
    // api2.emplace<engine::vulkan::Api>();
    // const auto apiIdx = api2.index();
    // const auto & api2Impl = std::get<engine::vulkan::Api>(api2);
    // using Api2 = std::decay_t<decltype(api2Impl)>;

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
    
    Api::Instance instance{instanceCreateInfo};

    if (!instance) {
        return 2;
    }

    engine::GLFWModule glfwModule;
    const auto & window0 = glfwModule.open(Api::name().data());

    Api::Surface surface{instance, window0.getSurfaceCreateInfo()};
    if (!surface) {
        return 3;
    }

    engine::rhi::PhysicalDeviceFeatureRequest gpuFeatures{};
    auto devices = Api::PhisicalDevice::getSuitableDevices(instance, {gpuFeatures, {Api::Impl::kSwapchainExtName}});

    if (devices[0].checkSurfaceSupport(surface)) {
        // const auto surfaceSupportDetails = engine::vulkan::getSurfaceSupportDetails(devices[0], surface);
        auto depthFormat = devices[0].getSupportedDepthFormat(32u, true);

        const auto queueFlags = engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present;
        auto [index, family] = devices[0].getQueueFamily(queueFlags);
        engine::rhi::DeviceCreateInfo deviceCreateInfo{gpuFeatures};
        deviceCreateInfo.queueFamilies.push_back({index, 1u, {}, queueFlags});
        deviceCreateInfo.commandPoolThreadsCount = 1u;
        
        Api::Device device(devices[0], deviceCreateInfo);

        engine::rhi::SwapchainCreateInfo swapchainCreateInfo = {};
        swapchainCreateInfo.imagesCount = 3u;
        swapchainCreateInfo.queueFamilies.push_back(index);

        Api::Swapchain swapchain(device, surface, swapchainCreateInfo);
        swapchain.resize(1024u, 768u, true);

        // const uint32_t frameBuffersCount = std::min(3u, swapchain.getImagesCount());
        const uint32_t frameBuffersCount = swapchain.getImagesCount();

        std::vector<Api::Image> depthImages;
        depthImages.reserve(frameBuffersCount);
        for (uint32_t i = 0u; i < frameBuffersCount; ++i) {
            depthImages.emplace_back(device, engine::rhi::ImageInfo{
                                            .type = engine::rhi::ImageType::eType_2D,
                                            .usage = engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment,
                                            .sharing = engine::rhi::SharingMode::eExclusive,
                                            .format = depthFormat,
                                            .aspectFlags = engine::rhi::ImageAspectFlags::eDepthStencil,
                                            .tiling = engine::rhi::ImageTiling::eOptimal,
                                            .flags = engine::rhi::ImageCreateFlags::eNone,
                                            .memoryFlags = engine::rhi::MemoryPropertyFlags::eDeviceLocal,
                                            .memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated,
                                            .extent = {1024u, 768u, 1u},
                                            .mipLevels = 1u,
                                            .arrayLayers = 1u,
                                            .samples = 1u,
                                            .queueFamilyIndices = {index}
                                        }
                                    );
        }

        auto renderPass = device.getRenderPass(createRenderPassDescription(swapchain.getFormat(), depthFormat));

        std::vector<Api::FrameBuffer> frameBuffers;
        frameBuffers.reserve(frameBuffersCount);
        for (uint32_t i = 0; i < frameBuffersCount; ++i) {
            auto & image = swapchain.getImage(i);
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};
            fbAttachemnts[0].image = &image;
            fbAttachemnts[0].imageSize = {image.getExtent().width, image.getExtent().height};

            auto & depthImage = depthImages[i];
            fbAttachemnts[1].image = &depthImage;
            fbAttachemnts[1].imageSize = {depthImage.getExtent().width, depthImage.getExtent().height};
            fbAttachemnts[1].aspect = engine::rhi::ImageAspectFlags::eDepthStencil;

            frameBuffers.emplace_back(device, std::move(fbAttachemnts), renderPass);
        }

        std::vector<Api::Device::Semaphore> presentReadySemaphores(frameBuffersCount);
        for (auto & semaphore : presentReadySemaphores) {
            semaphore = device.createSemaphore();
        }

        Api::Queue queue(device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, frameBuffersCount);
        std::vector<Api::CommandBuffer> cmdBuffers;

        // reserve frames data
        cmdBuffers.reserve(frameBuffersCount);
        device.getDescriptorPoolManager().useFrames(frameBuffersCount);
        engine::rhi::FrameIndex frameIndex{static_cast<uint8_t>(frameBuffersCount)};
        
        //
        auto program = loadShaders(device);
        gProgram0 = &program;
        auto pipeline0 = createPipeline(device, renderPass);
        pipeline = &pipeline0;
        auto buffer = createBuffers(device);
        vtxBuffer = &buffer;
        //
        bool renderInited = false;
        constexpr float inputCheckTime = 1.0f / 60.0f;
        constexpr float redrawTime = 1.0f / 120.0f;

        threads::Executor w([&](auto & executionContext, float delta) {
            executionContext.performTasks();
            if (renderInited) {
                const auto frameId = frameIndex();
                // static uint32_t a = 0u;
                // LOG_I("threadId_0: %d, delta: %f, value = %d\n", threads::unique::id(), delta, ++a);
                auto const waitSubmitResult = queue.waitSubmitComplete(frameId); // result!!!
                
                const auto [imageIndex, success] = swapchain.acquireNextImage(frameId);

                if (!success) {
                    return;
                }

                device.getDescriptorPoolManager().resetPools(frameId); // reset temp pools

                // if we refill cmdBuffers here - we can use less buffers count!

                // submit
                const auto waitSemaphores = std::array{swapchain.getImageAvailableSemaphore(frameId)};
                const auto signalSemaphores = std::array{presentReadySemaphores[frameId]};
                const auto waitStages = Api::Helper::convert2api(std::array{engine::rhi::PipelineStageFlags::eColor_Attachment_Output});

                const auto submitSyncInfo = Api::CommandBuffer::createSubmitSyncInfo(waitStages, waitSemaphores, signalSemaphores);
                auto const submitResult = queue.submit(std::array{queue.submitInfo(std::array{cmdBuffers[frameId]()}, submitSyncInfo)}, frameId);

                ++frameIndex;

                // present
                auto const presentResult = queue.present(std::array{swapchain()}, std::array{imageIndex}, signalSemaphores);
            }
        }, redrawTime);

        w.run();

        w.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            auto * commandPool = device.acquireCommandPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);

            /*const*/ auto clearValues = Api::Helper::convert2api(std::array{
                engine::rhi::ClearValue{0x77'77'77'ffu},
                engine::rhi::ClearValue{1.0f}
            });

            for (size_t i = 0; i < frameBuffersCount; ++i) {
                // if (i == 1) {
                //     clearValues[0].color = {{0.7f, 1.0f, 0.7f, 1.0f}};
                // } else if (i == 2) {
                //     clearValues[0].color = {{0.7f, 0.7f, 1.0f, 1.0f}};
                // }

                auto & commandBuffer = cmdBuffers.emplace_back(*commandPool, engine::rhi::CommandBufferLevel::ePrimary);
                commandBuffer.begin();

                commandBuffer.beginRenderPass(renderPass, frameBuffers[i], clearValues, {{0, 0}, {1024, 768}});

                commandBuffer.setViewport(0.0f, 0.0f, 1024.0f, 768.0f);
                commandBuffer.setScissorRect({{0, 0}, {1024, 768}});

                commandBuffer.bindPipeline(*pipeline);

                commandBuffer.bindVertexBuffers(std::array{(*vtxBuffer)()}, std::array{uint64_t{0u}});
                commandBuffer.draw(3u);
                
                commandBuffer.endRenderPass();
                commandBuffer.end();
            }
            renderInited = true;
        //    LOG_I("threadId_0: %d, \n", threads::unique_t<int>::id());
        }}));
    
        glfwModule.process(inputCheckTime);
    }

    return 1;
}