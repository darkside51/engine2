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

#include "graphics/render/rhi/common/api.h"
#include "graphics/render/rhi/common/utils.h"
#include "graphics/render/rhi/vulkan/api.h"
#include "graphics/render/rhi/vulkan/vk_types.h"
#include "graphics/render/rhi/vulkan/instance.h"
#include "graphics/render/rhi/vulkan/physical_device.h"
#include "graphics/render/rhi/vulkan/surface.h"
#include "graphics/render/rhi/vulkan/device.h"
#include "graphics/render/rhi/vulkan/swapchain.h"
#include "graphics/render/rhi/vulkan/image.h"
#include "graphics/render/rhi/vulkan/renderpass.h"
#include "graphics/render/rhi/vulkan/framebuffer.h"
#include "graphics/render/rhi/vulkan/command_pool.h"

engine::rhi::RenderPassDescription createRenderPassDescription(engine::rhi::Format colorFormat, engine::rhi::Format depthFormat) {
    std::vector<engine::rhi::SubpassDependency> dependencies(2u);
    dependencies[0].srcSubpass = engine::rhi::kSubpassExternal;
    dependencies[0].dstSubpass = 0u;
    dependencies[0].srcStageMask = engine::rhi::PipelineStageFlags::eColor_Attachment_Output; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    dependencies[0].dstStageMask = engine::rhi::PipelineStageFlags::eEarly_Fragment_Tests | engine::rhi::PipelineStageFlags::eColor_Attachment_Output; // (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT - https://vkguide.dev/docs/chapter-4/double_buffering/) is a loadOp stage for color attachments
    dependencies[0].srcAccessMask = engine::rhi::AccessFlags::eNone; // semaphore wait already does memory dependency for us
    dependencies[0].dstAccessMask = engine::rhi::AccessFlags::eColor_Attachment_Write; // is a loadOp CLEAR access mask for color attachments
    dependencies[0].dependencyFlags = engine::rhi::DependencyFlags::eBy_Region;

    dependencies[1].srcSubpass = 0u;
    dependencies[1].dstSubpass = engine::rhi::kSubpassExternal;
    dependencies[1].srcStageMask = engine::rhi::PipelineStageFlags::eColor_Attachment_Output; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    dependencies[1].dstStageMask = engine::rhi::PipelineStageFlags::eBottom_Of_Pipe; // Do not block any subsequent work
    dependencies[1].srcAccessMask = engine::rhi::AccessFlags::eColor_Attachment_Write; // semaphore wait already does memory dependency for us
    dependencies[1].dstAccessMask = engine::rhi::AccessFlags::eNone;         // is a loadOp CLEAR access mask for color attachments
    dependencies[1].dependencyFlags = engine::rhi::DependencyFlags::eBy_Region;

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

    engine::rhi::AttachmentReference colorReference;
    colorReference.attachment = 0u;
    colorReference.layout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;
    colorReference.aspectMask = engine::rhi::ImageAspectFlags::eColor;

    engine::rhi::AttachmentReference depthReference;
    depthReference.attachment = 1u;
    depthReference.layout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;
    depthReference.aspectMask = engine::rhi::ImageAspectFlags::eDepthStencil;

    std::vector<engine::rhi::SubpassDescription> subpasses{1};
    subpasses[0].colorAttachments.reserve(1u);
    subpasses[0].colorAttachments[0] = colorReference;
    subpasses[0].depthStencilAttachment = depthReference;

    // VkSubpassDescription subpassDescription = {};
    // subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // subpassDescription.colorAttachmentCount = 1;
    // subpassDescription.pColorAttachments = &colorReference;
    // subpassDescription.pDepthStencilAttachment = &depthReference;
    // subpassDescription.inputAttachmentCount = 0;
    // subpassDescription.pInputAttachments = nullptr;
    // subpassDescription.preserveAttachmentCount = 0;
    // subpassDescription.pPreserveAttachments = nullptr;
    // subpassDescription.pResolveAttachments = nullptr;

    return engine::rhi::RenderPassDescription{
        std::move(attachments), 
        std::move(subpasses), 
        std::move(dependencies)
    };
}

int main() {
    engine::common::Version v(0, 0, 4);
    engine::tests::execute_tests();

    engine::rhi::Api<engine::vulkan::Api> api;
    using Api = decltype(api);

    engine::rhi::InstanceCreateInfo instanceCreateInfo {
        .apiVersion = engine::common::Version{1, 3, 0},
        .extensions = {"VK_KHR_surface", "VK_KHR_win32_surface", "VK_KHR_xcb_surface", "VK_KHR_xlib_surface", 
		    "VK_KHR_wayland_surface", "VK_MVK_macos_surface", "VK_QNX_screen_surface", 
            Api::Impl::kDebugUtilExtName // for validation
            },
        .layers = {Api::Impl::kLayerValidationKHR}, // for validation
        .createDebugMessages = false,
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
        
        Api::Device device(devices[0], deviceCreateInfo);

        engine::rhi::SwapchainCreateInfo swapchainCreateInfo = {};
        swapchainCreateInfo.imagesCount = 3u;
        swapchainCreateInfo.queueFamilies.push_back(index);

        Api::Swapchain swapchain(device, surface, swapchainCreateInfo);
        swapchain.resize(1024u, 768u, true);

        Api::Image depthImage{device, {
            .type = engine::rhi::ImageType::eType_2D,
            .usage = engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment,
            .sharing = engine::rhi::SharingMode::eExclusive,
            .format = depthFormat,
            .aspectFlags = engine::rhi::ImageAspectFlags::eDepthStencil,
            .tiling = engine::rhi::ImageTiling::eOptimal,
            .flags = engine::rhi::ImageCreateFlags::eNone,
            .extent = {1024u, 768u, 1u},
            .mipLevels = 1u,
            .arrayLayers = 1u,
            .samples = 1u,
            .queueFamilyIndices = {index}
        }};

        Api::RenderPassManager renderPassManager{device};
        auto renderPass = renderPassManager.getRenderPass(createRenderPassDescription(swapchain.getFormat(), depthFormat));

        std::vector<Api::FrameBuffer> frameBuffers;
        frameBuffers.reserve(swapchain.getImagesCount());
        for (uint32_t i = 0; i < swapchain.getImagesCount(); ++i) {
            auto & image = swapchain.getImage(i);
            std::vector<engine::rhi::FrameBufferAttachment> fbAttachemnts{2u};
            fbAttachemnts[0].image = &image;
            fbAttachemnts[0].imageSize = {image.getExtent().width, image.getExtent().height};

            fbAttachemnts[1].image = &depthImage;
            fbAttachemnts[1].imageSize = {depthImage.getExtent().width, depthImage.getExtent().height};
            fbAttachemnts[1].aspect = engine::rhi::ImageAspectFlags::eDepthStencil;

            frameBuffers.emplace_back(device, std::move(fbAttachemnts), renderPass);
        }

        engine::vulkan::CommandPoolManager commandPoolManager{device, 1u};
        engine::vulkan::CommandPool * commandPool = nullptr;
        
        // vulkan code
        std::vector<VkCommandBuffer> cmdBuffers;
        cmdBuffers.reserve(swapchain.getImagesCount());

        std::vector<VkFence> submitCompleteFences(swapchain.getImagesCount());
        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (size_t i = 0; i < swapchain.getImagesCount(); ++i) {
            vkCreateFence(device(), &fenceCreateInfo, nullptr/*allocator*/, &submitCompleteFences[i]);
        }

        std::vector<VkSemaphore> presentReadySemaphores(swapchain.getImagesCount());
        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;
        for (auto & semaphore : presentReadySemaphores) {
            vkCreateSemaphore(device(), &semaphoreCreateInfo, nullptr/*allocator*/, &semaphore);
        }

        auto * queueFamily = device.findQueueFamily(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
        auto queue = queueFamily->queues[0];

        engine::rhi::FrameIndex frameIndex{static_cast<uint8_t>(swapchain.getImagesCount())};

        VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 1u;
        submitInfo.signalSemaphoreCount = 1u;
        submitInfo.pWaitDstStageMask = &submitPipelineStages;
        
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.swapchainCount = 1u;
		presentInfo.pSwapchains = &swapchain();
        presentInfo.waitSemaphoreCount = 1u;
		presentInfo.pResults = nullptr;

        threads::ExecutorImpl w([&](threads::Executor & w, float delta) {
            w.performTasks();
            if (commandPool) {
                // static uint32_t a = 0u;
                // LOG_I("threadId_0: %d, delta: %f, value = %d\n", threads::unique::id(), delta, ++a);

                const auto result = vkWaitForFences(device(), 1, &submitCompleteFences[frameIndex()], VK_TRUE, UINT64_MAX); 
                if (result != VK_SUCCESS) {
                    return;
                }
                vkResetFences(device(), 1, &submitCompleteFences[frameIndex()]);

                auto [imageIndex, success] = swapchain.acquireNextImage(frameIndex());

                if (!success) {
                    return;
                }

                submitInfo.commandBufferCount = 1u;
                submitInfo.pCommandBuffers = &cmdBuffers[frameIndex()];
                submitInfo.pWaitSemaphores = &swapchain.getImageAvailableSemaphore(frameIndex());
                submitInfo.pSignalSemaphores = &presentReadySemaphores[frameIndex()];
                
                const VkResult queueResult = vkQueueSubmit(queue, 1, &submitInfo, submitCompleteFences[frameIndex()]);

                presentInfo.pWaitSemaphores = &presentReadySemaphores[frameIndex()];
                presentInfo.pImageIndices = &imageIndex;

                VkResult presentResult = vkQueuePresentKHR(queue, &presentInfo);

                frameIndex.nextFrame();
            }
        }, 0.25f);
        w.run();
        w.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            commandPool = commandPoolManager.createPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);

            const std::array clearValues2 {
                engine::rhi::ClearValue{0x77'ff'77'ffu},
                engine::rhi::ClearValue{1.0f, 255}
            };

            auto clearValues = engine::vulkan::utils::convert2vk(clearValues2);

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.pNext = nullptr;

            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea = { {0, 0}, {1024, 768} };
            renderPassBeginInfo.clearValueCount = clearValues.size();
            renderPassBeginInfo.pClearValues = clearValues.data();

            VkCommandBufferBeginInfo beginInfo;
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = 0; // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - it here
			beginInfo.pInheritanceInfo = nullptr;

            const VkViewport viewport{0.0f, 0.0f, 1024.0f, 768.0f, 0.0f, 1.0f};
	        const VkRect2D scissor{{0, 0}, {1024, 768}};

            for (size_t i = 0; i < swapchain.getImagesCount(); ++i) {
                if (i > 0u) {
                    clearValues[0].color = {{0.25f + i * 0.25f, 0.25f, 0.25f, 1.0f}};
                }
                renderPassBeginInfo.pClearValues = clearValues.data();

                renderPassBeginInfo.framebuffer = frameBuffers[i]();

                auto & cmdBuff = cmdBuffers.emplace_back(commandPool->allocCommandBuffer(engine::rhi::CommandBufferLevel::ePrimary));
                auto result = vkBeginCommandBuffer(cmdBuff, &beginInfo);
                vkCmdBeginRenderPass(cmdBuff, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                vkCmdSetViewport(cmdBuff, 0, 1, &viewport);
                vkCmdSetScissor(cmdBuff, 0, 1, &scissor);

                vkCmdEndRenderPass(cmdBuff);
                result = vkEndCommandBuffer(cmdBuff);
            }
        //    LOG_I("threadId_0: %d, \n", threads::unique_t<int>::id());
        }}));
    
        glfwModule.process(1.0f / 60.0f);
    }

    return 1;
}