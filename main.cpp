#include "utils/profile/leaks.h"
#include "threads/executor.h"
#include "threads/worker.h"
#include "threads/utils.h"

#include "threads/thread_pool/thread_pool.h"

#include "utils/profile/time.h"
#include "common/version.h"

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
#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/inflight_buffer.h"
#include "graphics/ui/imgui.h"

#include "math/math.h"

#include "graphics/render/rhi/vulkan/runtime.h"

#include <imgui.h>

#include <cstdio>
#include <algorithm>

#include <glaze/glaze.hpp>

engine::FileManager fm;

engine::rhi::runtime::GpuProgramPtr programDepth = nullptr;
engine::rhi::runtime::GpuProgramPtr program = nullptr;
engine::rhi::runtime::GpuProgramPtr imguiProgram = nullptr;

engine::rhi::runtime::BufferPtr stagePositions = nullptr;
engine::rhi::runtime::BufferPtr stageColors = nullptr;
engine::rhi::runtime::BufferPtr positions = nullptr;
engine::rhi::runtime::BufferPtr colors = nullptr;

engine::rhi::runtime::BufferPtr stageIndices = nullptr;
engine::rhi::runtime::BufferPtr indices = nullptr;

std::unique_ptr<engine::graphics::InflightBuffer> uniformBufferI = nullptr;
engine::rhi::runtime::BufferPtr uniformBuffer2 = nullptr;

engine::rhi::runtime::BufferPtr stageImageBuffer = nullptr;
engine::rhi::runtime::ImagePtr sampledImage = nullptr;

void registerFileSystems() {
    fm.createFileSystem<engine::CommonFileSystem>("common_fs0", engine::CommonFileSystem::getCurrentDirectory() + "./resources/");
    fm.createFileSystem<engine::CommonFileSystem>("common_fs1", engine::CommonFileSystem::getCurrentDirectory() + "./resources/shaders/");
}

engine::rhi::RenderPassDescription createRenderPassDescription(engine::rhi::Format depthFormat) {
    std::vector<engine::rhi::AttachmentDescription> attachments(1u);
    attachments[0].format = depthFormat;
    attachments[0].loadStoreOp = {engine::rhi::AttachmentLoadOp::eClear, engine::rhi::AttachmentStoreOp::eStore};
    attachments[0].stencilLoadStoreOp = {engine::rhi::AttachmentLoadOp::eDont_Care, engine::rhi::AttachmentStoreOp::eDont_Care};
    attachments[0].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;

    std::vector<engine::rhi::SubpassDescription> subpasses(1u);
    subpasses[0].depthStencilAttachment = {
        .pNext = {nullptr, 0u},
        .attachment = 0u,
        .layout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal,
        .aspectMask = engine::rhi::ImageAspectFlags::eDepthStencil
    };

    return engine::rhi::RenderPassDescription{
        std::move(attachments), 
        std::move(subpasses), 
        {} // std::vector<engine::rhi::SubpassDependency>
    };
}

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
    deps[0].dependencyFlags = engine::rhi::DependencyFlags::eBy_Region; // ? need
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

void loadShaders(const std::unique_ptr<engine::rhi::runtime::Rhi> & api, 
    engine::rhi::runtime::DevicePtr & device, 
    engine::rhi::runtime::ShaderModulesManagerPtr & shaderModulesManager) {

    {
        auto const vBytes = fm.readFile("shaders/simple_depth.vert.spv");
        const auto & vsModule = shaderModulesManager->registerModule("simple_depth.vert", *device, engine::rhi::shaders::Stages::eVertex, vBytes);
        programDepth = api->createProgram(device, std::array{vsModule.getView()}, {.dynamicDescriptorsMask = 0b10u});
    }

    {
        auto const vBytes = fm.readFile("shaders/simple.vert.spv");
        auto const fBytes = fm.readFile("shaders/simple.frag.spv");

        const auto & vsModule = shaderModulesManager->registerModule("simple.vert", *device, engine::rhi::shaders::Stages::eVertex, vBytes); // call overloaded function
        const auto & fsModule = shaderModulesManager->registerModule("simple.frag", *device, engine::rhi::shaders::Stages::eFragment, fBytes);

        program = api->createProgram(device, std::array{vsModule.getView(), fsModule.getView()}, {.dynamicDescriptorsMask = 0b10u});
    }

    {
        auto const vBytes = fm.readFile("imgui.vert.spv");
        auto const fBytes = fm.readFile("imgui.frag.spv");

        const auto & vsModule = shaderModulesManager->registerModule("imgui.vert", *device, engine::rhi::shaders::Stages::eVertex, vBytes); // call overloaded function
        const auto & fsModule = shaderModulesManager->registerModule("imgui.frag", *device, engine::rhi::shaders::Stages::eFragment, fBytes);

        imguiProgram = api->createProgram(device, std::array{vsModule.getView(), fsModule.getView()});
    }
}

auto createPipelineDepth(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device,
    engine::rhi::runtime::RenderPass renderPass,
    const engine::rhi::runtime::GpuProgramPtr & program, 
    uint32_t subpass = 0u) {
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

   return api->createGraphicsPipeline(device, renderPass, program, info, subpass);
}

auto createPipeline(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device,
    engine::rhi::runtime::RenderPass renderPass,
    const engine::rhi::runtime::GpuProgramPtr & program, 
    uint32_t subpass = 0u) {
    using namespace engine::rhi;

    GraphicsPipelineStatesInfo info;
    info.viewportState = {1u, 1u};
    info.blendModes = {common_blend_modes::blend_none};
    // info.depthStencilState = {
    //     {
    //         .depthTestEnable = true,
    //         .depthWriteEnable = true,
    //         .depthCompareOp = CompareOp::eLess
    //     }, 
    //     {}
    // };

    // for depth prepass!
    info.depthStencilState = {
        {
            .depthTestEnable = true,
            .depthWriteEnable = false,
            .depthCompareOp = CompareOp::eEqual
        }, 
        {}
    };
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

   return api->createGraphicsPipeline(device, renderPass, program, info, subpass);
}

void createBuffers(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device) {
    using namespace engine::rhi;

    // vertices
    const BufferInfo stageInfo {
        BufferUsageFlags::eTransfer_Src,
        MemoryPropertyFlags::eHostVisible
    };

    stagePositions = api->createBuffer(device, stageInfo, sizeof(std::array<float, 3>) * 4u);
    {
        auto * map = stagePositions->map2<std::array<float, 3>>();
        map[0u] = {-0.5f, 0.5f, 0.0f};
        map[1u] = {0.5f, 0.5f, 0.0f};
        map[2u] = {0.0f, -0.5f, 0.0f};
        map[3u] = {-1.0f, -0.5f, 0.0f};
        stagePositions->flush();
        stagePositions->unmap();
    }

    stageColors = api->createBuffer(device, stageInfo, sizeof(uint32_t) * 4u);
    {
        auto * map = stageColors->map2<uint32_t>();
        map[0u] = 0xff'00'ff'00u; map[1u] = 0xff'00'00'ffu;
        map[2u] = 0xff'ff'00'00u; map[3u] = 0xff'ff'ff'ffu;
        stageColors->flush();
        stageColors->unmap();
    }

    const BufferInfo localInfo {
        BufferUsageFlags::eVertex_Buffer | BufferUsageFlags::eTransfer_Dst,
        MemoryPropertyFlags::eDeviceLocal
    };

    positions = api->createBuffer(device, localInfo, stagePositions->sizeInBytes());
    colors = api->createBuffer(device, localInfo, stageColors->sizeInBytes());

    // indices
    stageIndices = api->createBuffer(device, stageInfo, sizeof(uint16_t) * 6u);
    {
        auto * map = stageIndices->map2<uint16_t>();
        map[0u] = 0u;
        map[1u] = 1u;
        map[2u] = 2u;

        map[3u] = 0u;
        map[4u] = 2u;
        map[5u] = 3u;
        stageIndices->flush();
        stageIndices->unmap();
    }

    const BufferInfo localIndicesInfo {
        BufferUsageFlags::eIndex_Buffer | BufferUsageFlags::eTransfer_Dst,
        MemoryPropertyFlags::eDeviceLocal
    };

    indices = api->createBuffer(device, localIndicesInfo, stageIndices->sizeInBytes());

    // uniforms
    BufferInfo unifromsInfo {
        BufferUsageFlags::eUniform_Buffer,
        MemoryPropertyFlags::eHostVisible
    };

    uniformBuffer2 = api->createBuffer(device, unifromsInfo, sizeof(engine::vec2f));
    {
        auto * map = uniformBuffer2->map2<engine::vec2f>();
        map[0u] = {0.0f, 0.25f};
        uniformBuffer2->flush();
        uniformBuffer2->unmap();
    }

    unifromsInfo.memoryFlags = MemoryPropertyFlags::eDeviceLocal;
    unifromsInfo.usage = unifromsInfo.usage | BufferUsageFlags::eTransfer_Dst;

    const size_t kUniformBufferOffsetAlign = device->getUniformBufferOffsetAlignment();
    uniformBufferI = std::make_unique<engine::graphics::InflightBuffer>(api, device, unifromsInfo, std::max(kUniformBufferOffsetAlign, sizeof(engine::vec2f)) * 2u);
    {
        auto * map = uniformBufferI->map2<engine::vec2f>();
        map[0u] = {-0.5f, 0.0};
        map[kUniformBufferOffsetAlign / sizeof(engine::vec2f) + 0u] = {0.0f, -0.25f};
        uniformBufferI->unmap();
    }
}

void createImageData(const std::unique_ptr<engine::rhi::runtime::Rhi> & api,
    std::unique_ptr<engine::rhi::runtime::Device> & device) {
    using namespace engine::rhi;

    const BufferInfo stageInfo {
        BufferUsageFlags::eTransfer_Src,
        MemoryPropertyFlags::eHostVisible
    };

    stageImageBuffer = api->createBuffer(device, stageInfo, sizeof(uint32_t) * 4u);
    {
        auto * map = stageImageBuffer->map2<uint32_t>();
        map[0u] = 0xff'ff'ff'ffu; // -> abgr
        map[1u] = 0xff'55'55'55u;
        map[2u] = 0xff'55'55'55u;
        map[3u] = 0xff'ff'ff'ffu;
        stageImageBuffer->flush();
        stageImageBuffer->unmap();
    }

    const auto imageInfo = ImageInfo{
        ImageUsageFlags::eSampled | ImageUsageFlags::eTransfer_Dst,
        Format::eR8G8B8A8_Unorm, {2u, 2u, 1u}
    };

    sampledImage = api->createImage(device, imageInfo);
}

struct my_struct {
    int i = 287;
    double d = 3.14;
    std::string hello = "Hello World";
    std::array<uint64_t, 3> arr = {1, 2, 3};
};

int main() {
    
    my_struct obj{};

    constexpr std::string_view input = R"({"i":123,"d":2.72,"hello":"Hello World!","arr":[4,5,6]})";
    auto err = glz::read_json(obj, input);

    glz::json_t json{};
    err = glz::read_json(json, input);
    const auto d_val = json.at("d").as<float>();
    const auto i_val = json["i"].as<int>();
    const bool hasHello = json.contains("hello");
    const bool hasHello2 = json.contains("hello2");

    if (err) {}



    // engine::tests::execute_tests();

    // todo: 
    // 1. impl staging buffer for inflightBuffer - in progress
    // 2. simplify
    // 3. optimize
    // 4. rhi::Result
    // 5. refactoring

    registerFileSystems();

    engine::GLFWModule glfwModule;
    
    const engine::rhi::Api<engine::vulkan::Api> api;
    using Api = decltype(api);

    auto & window0 = glfwModule.createWindow(Api::name().data(), 1024u, 768u);
    window0.setVisible(false);

#ifndef NDEBUG
    bool const kUseApiDebugMessages = true;
    std::vector<std::string_view> extensions = {"VK_KHR_surface", "VK_KHR_win32_surface", 
        "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
        "VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface", 
        Api::Impl::kDebugUtilExtName // for validation
    };
    std::vector<std::string_view> layers = {Api::Impl::kLayerValidationKHR};
#else
    bool const kUseApiDebugMessages = false;
    std::vector<std::string_view> extensions = {"VK_KHR_surface", "VK_KHR_win32_surface", 
        "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
        "VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface"
    };
    std::vector<std::string_view> layers = {};
#endif

    engine::rhi::InstanceCreateInfo instanceCreateInfo {
        .apiVersion = engine::common::Version{1, 3, 0},
        .extensions = extensions,
        .layers = layers,
        .createDebugMessages = kUseApiDebugMessages,
        .allocationCallback = nullptr
    };

    std::unique_ptr<engine::rhi::runtime::Rhi> rApi = std::make_unique<engine::vulkan::runtime::Rhi>(instanceCreateInfo);
    
    engine::rhi::PhysicalDeviceFeatureRequest gpuFeatures{};
    auto gpuDevices = rApi->enumerateDevices({
        gpuFeatures, 
        {
            engine::vulkan::runtime::Rhi::Api::Impl::kSwapchainExtName, 
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
        }
    });

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

        auto [width, height] = window0.getSize();
        swapchain->resize(width, height, false);
        const uint32_t swapchainImagesCount = swapchain->getImagesCount();

        // auto depthPrePass = rApi->getRenderPass(device, createRenderPassDescription(depthFormat));
        // auto renderPass = rApi->getRenderPass(device, createRenderPassDescription(swapchain->getFormat(), depthFormat));

        auto renderPass = rApi->getRenderPass(device, createRenderPassDescription2(swapchain->getFormat(), depthFormat));
        
        auto imageInfo = engine::rhi::ImageInfo{
            engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment,
            depthFormat, {width, height, 1u}
        }; 
        imageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

        std::vector<engine::rhi::runtime::ImagePtr> depthImages;
        depthImages.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            depthImages.emplace_back(std::move(rApi->createImage(device, imageInfo)));
        }

        // std::vector<engine::rhi::runtime::ImagePtr> depthImages2;
        // depthImages2.reserve(swapchainImagesCount);
        // for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
        //     depthImages2.emplace_back(std::move(rApi->createImage(device, imageInfo)));
        // }

        std::vector<engine::rhi::runtime::FrameBufferPtr> frameBuffers;
        frameBuffers.reserve(swapchainImagesCount);
        for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
            auto & image = swapchain->getImage(i);
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};
            fbAttachemnts[0u].image = &image;
            fbAttachemnts[0u].imageSize = {image.getExtent().width, image.getExtent().height};
            fbAttachemnts[0u].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

            auto & depthImage = depthImages[i];
            fbAttachemnts[1u].image = depthImage.get();
            fbAttachemnts[1u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
            fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?
            fbAttachemnts[1u].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

            frameBuffers.emplace_back(rApi->createFrameBuffer(device, std::move(fbAttachemnts), renderPass));
        }

        // std::vector<engine::rhi::runtime::FrameBufferPtr> frameBuffers2;
        // frameBuffers2.reserve(swapchainImagesCount);
        // for (uint32_t i = 0u; i < swapchainImagesCount; ++i) {
        //     auto & depthImage = depthImages2[i];
        //     std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{1u};
        //     fbAttachemnts[0u].image = depthImage.get();
        //     fbAttachemnts[0u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
        //     fbAttachemnts[0u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?
        //     fbAttachemnts[0u].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

        //     frameBuffers2.emplace_back(rApi->createFrameBuffer(device, std::move(fbAttachemnts), depthPrePass));
        // }

        auto queue = rApi->getQueue(device, engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present, 0u, inflightFrames);

        std::vector<engine::rhi::runtime::Semaphore> readyToPresentSemaphores(inflightFrames);
        for (auto & semaphore : readyToPresentSemaphores) {
            semaphore = rApi->createSemaphore(device);
        }

        auto shaderModulesManager = rApi->createShaderModulesManager();
        loadShaders(rApi, device, shaderModulesManager);
        // auto pipeline = createPipeline(rApi, device, renderPass, program);
        // auto depthPipeline = createPipelineDepth(rApi, device, depthPrePass, programDepth);

        auto pipeline = createPipeline(rApi, device, renderPass, program, 1u);
        auto depthPipeline = createPipelineDepth(rApi, device, renderPass, programDepth, 0u);

        createBuffers(rApi, device);
        createImageData(rApi, device);

        std::vector<engine::rhi::runtime::CommandListPtr> commandLists;
        
        constexpr float inputCheckTime = 1.0f / 60.0f;
        constexpr float redrawTime = 1.0f / 120.0f;

        constexpr auto clearValues = std::array{
            engine::rhi::ClearValue{0x77'77'77'ffu},
            engine::rhi::ClearValue{1.0f}
        };

        // constexpr auto clearValuesD = std::array{
        //     engine::rhi::ClearValue{1.0f}
        // };

        engine::rhi::FrameIndex frameIndex{inflightFrames};

        const size_t kUniformBufferOffsetAlign = device->getUniformBufferOffsetAlignment();
        bool canDraw = true;

        engine::ImGuiSystem imgui;
        imgui.createPipeline(rApi, device, imguiProgram, renderPass, 1u);
        imgui.createFontTexture(rApi, device);
        imgui.createBuffers(rApi, device);

        threads::Executor executor([&](auto & executionContext, float delta) {
            executionContext.performTasks();
            if (!commandLists.empty()) {

                imgui.beginFrame(delta, width, height);
                ImGui::ShowDemoWindow();
                imgui.endFrame();

                const auto frameId = frameIndex();

                if (canDraw) {
                    auto const waitSubmitResult = queue->waitSubmit(frameId); // result!!!
                }

                const auto [imageIndex, success] = swapchain->acquireNextImage(frameId); // https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation

                if (!success) {
                    canDraw = false;
                    return;
                }

                canDraw = true;
                device->resetFrameTmpData(frameId);

                // render frame
                auto & commandList = commandLists[frameId];
                commandList->begin(engine::rhi::CommandBufferUsageFlags::eOne_Time_Submit);

                if (stagePositions) {
                    commandList->copyBuffer2Buffer(stagePositions, positions, 
                        {.srcOffset = 0u, .dstOffset = 0u, .size = stagePositions->sizeInBytes()});

                    commandList->copyBuffer2Buffer(stageColors, colors, 
                        {.srcOffset = 0u, .dstOffset = 0u, .size = stageColors->sizeInBytes()});

                    commandList->copyBuffer2Buffer(stageIndices, indices, 
                        {.srcOffset = 0u, .dstOffset = 0u, .size = stageIndices->sizeInBytes()});

                    stagePositions = nullptr;
                    stageColors = nullptr;
                    stageIndices = nullptr;
                }

                if (stageImageBuffer) {
                    commandList->copyBuffer2Image(stageImageBuffer, sampledImage, 
                        {
                         .bufferOffset = 0u,
                         .bufferRowLength = 0u,
                         .bufferImageHeight = 0u,
                         .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                         .imageOffset = {0u, 0u, 0u},
                         .imageExtent = sampledImage->getExtent()
                        }
                    );
                    stageImageBuffer = nullptr;
                }

                { // update uniforms
                    auto * map = uniformBufferI->map2<engine::vec2f>();

                    if (map[0u].x > 1.5f) { map[0u].x = -1.5f; }
                    map[0u].x += 1.0f * delta;
      
                    uniformBufferI->unmap();
                    uniformBufferI->commit(commandList, frameId);
                }

                imgui.prepare(commandList, frameId, rApi, device);
                ///////////////////////////
                // commandList->beginRenderPass(depthPrePass, frameBuffers2[imageIndex], clearValuesD, {{0, 0}, {width, height}});
                commandList->beginRenderPass(renderPass, frameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}});
                commandList->bindPipeline(depthPipeline);

                commandList->setViewport(0.0f, 0.0f, width, height);
                commandList->setScissor({{0, 0}, {width, height}});

                // first triangle
                commandList->bindBuffer(0u, (*uniformBufferI)(frameId), 0u, 
                    0u, sizeof(engine::vec2f));
                commandList->bindBuffer(1u, (*uniformBufferI)(frameId), 0u, 
                    engine::align_value(sizeof(engine::vec2f), kUniformBufferOffsetAlign), sizeof(engine::vec2f));

                commandList->bindImage(0u, sampledImage, engine::rhi::SamplerInfo{}, 1u);   

                commandList->bindVertexBuffer(0u, positions);
                commandList->draw(3u);

                // second triangle
                commandList->bindBuffer(1u, (*uniformBufferI)(frameId), 0u, 0u, sizeof(float) * 2u);
                commandList->bindIndexBuffer(indices, 0u, engine::rhi::IndexType::eU16);
                commandList->drawIndexed(6u);

                commandList->nextSubpass(engine::rhi::SubpassContents::ePrimaryBufferInline);
                // commandList->endRenderPass();
                //////////////////////////

                // commandList->beginRenderPass(renderPass, frameBuffers[imageIndex], clearValues, {{0, 0}, {width, height}});

                commandList->bindPipeline(pipeline);

                commandList->setViewport(0.0f, 0.0f, width, height);
                commandList->setScissor({{0, 0}, {width, height}});

                // first triangle
                commandList->bindBuffer(0u, (*uniformBufferI)(frameId), 0u, 
                    0u, sizeof(engine::vec2f));
                commandList->bindBuffer(1u, (*uniformBufferI)(frameId), 0u, 
                    engine::align_value(sizeof(engine::vec2f), kUniformBufferOffsetAlign), sizeof(engine::vec2f));

                commandList->bindImage(0u, sampledImage, engine::rhi::SamplerInfo{}, 1u);   

                commandList->bindVertexBuffer(0u, positions);
                commandList->bindVertexBuffer(1u, colors);
                commandList->draw(3u);

                // second triangle
                // commandList->bindBuffer(1u, uniformBuffer2);
                commandList->bindBuffer(1u, (*uniformBufferI)(frameId), 0u, 0u, sizeof(float) * 2u);
                commandList->bindIndexBuffer(indices, 0u, engine::rhi::IndexType::eU16);
                commandList->drawIndexed(6u);
                
                imgui.draw(commandList, frameId);

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

        executor.run();

        executor.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            auto * commandPool = device->acquireCommandPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
            commandLists.reserve(inflightFrames);
            for (size_t i = 0u; i < inflightFrames; ++i) {
                commandLists.emplace_back(rApi->createCommandList(commandPool, engine::rhi::CommandBufferLevel::ePrimary));
            }
        }}));

        window0.setVisible(true);

        engine::InputHandler inputHandler;
        inputHandler.addObserver(&imgui);

        window0.setOnPointer([&](engine::PointerEvent && event) {
            inputHandler.onPointerEvent(std::move(event));
        });

        window0.setOnWheel([&](float dx, float dy) {
            inputHandler.onWheelEvent(dx, dy);
        });

        window0.setOnKey([&](engine::KeyEvent && event, uint8_t mask) {
            if (event.key == engine::KeyboardKey::eEscape) {
                window0.close();
                return;
            }
            inputHandler.onKeyEvent(std::move(event), mask);
        });

        window0.setOnResize([&](uint16_t w, uint16_t h) {
            executor.pause();
            queue->waitIdle();
            device->waitIdle();

            // resize swapchain
            swapchain->resize(w, h, false);
            const uint32_t swapchainImagesCountNew = swapchain->getImagesCount();

            auto const size = swapchain->getSize();
            width = size.width;
            height = size.height;

            // recreate resources
            auto imageInfo = engine::rhi::ImageInfo{
                engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment,
                depthFormat, {width, height, 1u}
            }; 
            imageInfo.memoryExtFlags = engine::rhi::MemoryExtendedFlags::eDedicated;

            depthImages.clear();
            depthImages.reserve(swapchainImagesCountNew);
            for (uint32_t i = 0u; i < swapchainImagesCountNew; ++i) {
                depthImages.emplace_back(std::move(rApi->createImage(device, imageInfo)));
            }

            frameBuffers.clear();
            frameBuffers.reserve(swapchainImagesCountNew);
            for (uint32_t i = 0u; i < swapchainImagesCountNew; ++i) {
                auto & image = swapchain->getImage(i);
                std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};
                fbAttachemnts[0u].image = &image;
                fbAttachemnts[0u].imageSize = {image.getExtent().width, image.getExtent().height};
                fbAttachemnts[1u].finalLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

                auto & depthImage = depthImages[i];
                fbAttachemnts[1u].image = depthImage.get();
                fbAttachemnts[1u].imageSize = {depthImage->getExtent().width, depthImage->getExtent().height};
                fbAttachemnts[1u].aspect = engine::rhi::ImageAspectFlags::eDepth;  // can get it from depthImage?
                fbAttachemnts[1u].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal; // equal to subpasses attachments layouts, maybe can get it from subpasses?

                frameBuffers.emplace_back(rApi->createFrameBuffer(device, std::move(fbAttachemnts), renderPass));
            }

            executor.resume();
        });

        window0.setOnClose([&]() {
            executor.stop();
            device->waitIdle();

            programDepth = nullptr;
            program = nullptr;
            imguiProgram = nullptr;

            stagePositions = nullptr;
            stageColors = nullptr;
            stageIndices = nullptr;

            positions = nullptr;
            colors = nullptr;
            indices = nullptr;

            uniformBufferI = nullptr;
            uniformBuffer2 = nullptr;

            stageImageBuffer = nullptr;
            sampledImage = nullptr;

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