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
#include "graphics/render/rhi/vulkan/api.h"
#include "graphics/render/rhi/vulkan/instance.h"
#include "graphics/render/rhi/vulkan/physical_device.h"
#include "graphics/render/rhi/vulkan/surface.h"
#include "graphics/render/rhi/vulkan/device.h"
#include "graphics/render/rhi/vulkan/swapchain.h"
#include "graphics/render/rhi/vulkan/image.h"
#include "graphics/render/rhi/vulkan/renderpass.h"
#include "graphics/render/rhi/vulkan/framebuffer.h"
#include "graphics/render/rhi/vulkan/command_pool.h"

int main() {
    engine::common::Version v(0, 0, 4);
    engine::tests::execute_tests();

    engine::GLFWModule glfwModule;
    const auto & window0 = glfwModule.open("window0");

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

    Api::Surface surface{instance, window0.getSurfaceCreateInfo()};
    if (!surface) {
        return 3;
    }

    engine::rhi::PhysicalDeviceFeatureRequest gpuFeatures{};
    auto devices = Api::PhisicalDevice::getSuitableDevices(instance, {gpuFeatures, {Api::Impl::kSwapchainExtName}});

    if (devices[0].checkSurfaceSupport(surface)) {
        const auto surfaceSupportDetails = engine::vulkan::getSurfaceSupportDetails(devices[0], surface);
        auto depthFormat = devices[0].getSupportedDepthFormat(32u, true);

        auto [index, family] = devices[0].getQueueFamily(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);
        engine::rhi::DeviceCreateInfo deviceCreateInfo{gpuFeatures};
        deviceCreateInfo.queueFamilies.push_back({index, 1u, {}});
        
        Api::Device device(devices[0], deviceCreateInfo);

        engine::rhi::SwapchainCreateInfo swapchainCreateInfo = {};
        swapchainCreateInfo.imagesCount = 3u;
        swapchainCreateInfo.queueFamilies.push_back(index);

        Api::Swapchain swapchain(device, surface, swapchainCreateInfo);
        swapchain.resize(1024u, 768u, true);

        auto depthImageInfo = engine::rhi::ImageInfo{};
        depthImageInfo.usage = engine::rhi::ImageUsageFlags::eDepth_Stencil_Attachment;
        depthImageInfo.extent = {1024u, 768u, 1u};
        depthImageInfo.format = depthFormat;
        depthImageInfo.tiling = engine::rhi::ImageTiling::eOptimal;
        depthImageInfo.aspectFlags = engine::rhi::ImageAspectFlags::eDepthStencil;
        depthImageInfo.queueFamilyIndices = {index};
        Api::Image depthImage(device, depthImageInfo);

        engine::vulkan::RenderPassManager renderPassManager{device};

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
        attachments[0].format = swapchain.getFormat();
        attachments[0].loadOp = engine::rhi::AttachmentLoadOp::eClear;
        attachments[0].storeOp = engine::rhi::AttachmentStoreOp::eStore;
        attachments[0].stencilLoadOp = engine::rhi::AttachmentLoadOp::eDont_Care;
        attachments[0].stencilStoreOp = engine::rhi::AttachmentStoreOp::eDont_Care;
        attachments[0].initialLayout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;
        attachments[0].finalLayout = engine::rhi::ImageLayout::ePresent_Src;

        attachments[1].format = depthFormat;
        attachments[1].loadOp = engine::rhi::AttachmentLoadOp::eClear;
        attachments[1].storeOp = engine::rhi::AttachmentStoreOp::eDont_Care;
        attachments[1].stencilLoadOp = engine::rhi::AttachmentLoadOp::eDont_Care;
        attachments[1].stencilStoreOp = engine::rhi::AttachmentStoreOp::eDont_Care;
        attachments[1].initialLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;
        attachments[1].finalLayout = engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;

        engine::rhi::AttachmentReference colorReference;
        colorReference.attachment = 0;
        colorReference.layout = engine::rhi::ImageLayout::eColor_Attachment_Optimal;
        colorReference.aspectMask = engine::rhi::ImageAspectFlags::eColor;

        engine::rhi::AttachmentReference depthReference;
        depthReference.attachment = 1;
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


        engine::rhi::RenderPassDescription renderPassDescription{
            std::move(attachments), 
            std::move(subpasses), 
            std::move(dependencies)
        };

        auto renderPass = renderPassManager.getRenderPass(renderPassDescription);

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
        std::vector<VkCommandBuffer> cmdBuffers;
        cmdBuffers.reserve(swapchain.getImagesCount());

        std::vector<VkFence> waitFences(swapchain.getImagesCount());
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = nullptr;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < swapchain.getImagesCount(); ++i) {
            vkCreateFence(device(), &fenceCreateInfo, nullptr/*allocator*/, &waitFences[i]);
        }

        std::vector<VkSemaphore> presentReadySemaphores(swapchain.getImagesCount());
        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;
        for (auto & semaphore : presentReadySemaphores) {
            vkCreateSemaphore(device(), &semaphoreCreateInfo, nullptr/*allocator*/, &semaphore);
        }

        const auto & queueFamilies = device.getQueueFamilies();
        auto queue = queueFamilies[0].queues[0];

        uint32_t frameIdx = 0u;

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

                const auto result = vkWaitForFences(device(), 1, &waitFences[frameIdx], VK_TRUE, 5000/*UINT64_MAX*/); 
                if (result != VK_SUCCESS) {
                    return;
                }

                auto [imageIndex, success] = swapchain.acquireNextImage(frameIdx);

                if (!success) {
                    return;
                }

                vkResetFences(device(), 1, &waitFences[frameIdx]);

                submitInfo.commandBufferCount = 1u;
                submitInfo.pCommandBuffers = &cmdBuffers[frameIdx];
                submitInfo.pWaitSemaphores = &swapchain.getImageAvailableSemaphore(frameIdx);
                submitInfo.pSignalSemaphores = &presentReadySemaphores[frameIdx];
                
                const VkResult queueResult = vkQueueSubmit(queue, 1, &submitInfo, waitFences[frameIdx]);

                presentInfo.pWaitSemaphores = &presentReadySemaphores[frameIdx];
                presentInfo.pImageIndices = &imageIndex;

                VkResult presentResult = vkQueuePresentKHR(queue, &presentInfo);

                frameIdx = (frameIdx + 1u) % swapchain.getImagesCount();
            }
        }, 1.0f);
        w.run();
        w.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            commandPool = commandPoolManager.createPool(engine::rhi::queue_flags::graphics | engine::rhi::queue_flags::present);

            VkClearValue clearValues[2];
            clearValues[0].color = {{0.25f, 0.25f, 0.25f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.pNext = nullptr;

            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea = { {0, 0}, {1024, 768} };
            renderPassBeginInfo.clearValueCount = 2;
            renderPassBeginInfo.pClearValues = clearValues;

            VkCommandBufferBeginInfo beginInfo;
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = nullptr;
			beginInfo.flags = 0; // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - it here
			beginInfo.pInheritanceInfo = nullptr;

            const VkViewport viewport{0.0f, 0.0f, 1024.0f, 768.0f, 0.0f, 1.0f};
	        const VkRect2D scissor{{0, 0}, {1024, 768}};

            for (size_t i = 0; i < swapchain.getImagesCount(); ++i) {
                clearValues[0].color = {{0.25f + i * 0.25f, 0.25f, 0.25f, 1.0f}};
                renderPassBeginInfo.pClearValues = clearValues;

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