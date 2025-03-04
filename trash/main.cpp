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

int main() {
    engine::common::Version v(0, 0, 4);
    engine::tests::execute_tests();

    engine::GLFWModule glfwModule;
    const auto & window0 = glfwModule.open("window0");
    glfwModule.open("window1");

    threads::ExecutorImpl w([](threads::Executor & w, float delta) {
        w.performTasks();
        static uint32_t a = 0u;
        LOG_I("threadId_0: %d, delta: %f, value = %d\n", threads::unique::id(), delta, ++a);
    }, 1.0f);
    w.run();
    w.emplaceTasks(stl::make_vector(threads::WorkerTask{[](){
       LOG_I("threadId_0: %d, \n", threads::unique_t<int>::id());
    }}));

    threads::ExecutorImpl w2([](threads::Executor & w, float delta) {
        LOG_I("threadId_1: %d\n", threads::unique::id());
    }, 0.5f);
    w2.run();

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

        std::vector<engine::rhi::SubpassDescription> subpasses{};

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

            frameBuffers.emplace_back(device, std::move(fbAttachemnts), renderPass);
        }


        // auto imageInfo = engine::rhi::ImageInfo{};
        // imageInfo.usage = engine::rhi::ImageUsageFlags::eSampled | engine::rhi::ImageUsageFlags::eTransfer_Dst;
        // imageInfo.extent = {1, 1, 1};
        // imageInfo.format = engine::rhi::Format::eR8G8B8A8_Unorm;
        // imageInfo.tiling = engine::rhi::ImageTiling::eOptimal;

        // engine::vulkan::Image image(device, imageInfo);
        // image.changeLayout(VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, 
        //                    VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        //                    VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 
        //                    VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        //                    VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 1, 0, 1, 0, 1);

        // image.changeLayout(VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, 
        //                 VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        //                 VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 
        //                 VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        //                 VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 1, 0, 1, 0, 1);

        int a = 0;
    }

    if (devices[1].checkSurfaceSupport(surface)) {
        const auto surfaceSupportDetails = engine::vulkan::getSurfaceSupportDetails(devices[1], surface);
    }
    
    engine::rhi::MemoryPropertyFlags f = engine::rhi::MemoryPropertyFlags::eDeviceLocal;
    engine::rhi::MemoryPropertyFlags g = engine::rhi::MemoryPropertyFlags::eHostVisible;
    auto vvv2 = f | g;

    glfwModule.process(1.0f / 60.0f);

    return 1;

    {
        using PAT2 = allocators::PoolAllocatorChain<sizeof(uint32_t), alignof(uint32_t), true>;
        PAT2 poolChain2(2u, 100u);

        std::vector<std::thread> threads;
        threads.reserve(200);
        std::array<uint32_t, 200> values;
        std::array<uint32_t*, 200> valuesPtrs;

        for (size_t i = 0; i < 200; ++i) {
            threads.emplace_back([&poolChain2, i, &values, &valuesPtrs]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
                auto * mem = poolChain2.alloc();
                auto * val = new(mem) uint32_t(i + 1);
                values[i] = *val;
                valuesPtrs[i] = val;
                printf("%zu : %d\n", i, *val);
            });
        }

        for (auto & t : threads) {
            t.join();
        }

        threads.clear();
        threads.reserve(101);

        for (size_t i = 99; i < 200; ++i) {
            threads.emplace_back([&poolChain2, i, &values, &valuesPtrs]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
                poolChain2.free(valuesPtrs[i]);
            });
        }

        for (auto & t : threads) {
            t.join();
        }


        threads.clear();
        threads.reserve(101);
        for (size_t i = 99; i < 200; ++i) {
            threads.emplace_back([&poolChain2, i, &values, &valuesPtrs]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
                auto * mem = poolChain2.alloc();
                auto * val = new(mem) uint32_t(i + 1);
            });
        }

        for (auto & t : threads) {
            t.join();
        }

        size_t i = 0u;
        for (auto & v : values) {
            printf("%zu : %d\n", i++, v);
        }

        printf("complete\n");
    }

    threads::ThreadPool tp(4u, 1024u, 64u);
    int c = 0;//sizeof(tp);
    // std::vector<threads::Task> tasks;
    // tasks.reserve(101);
    uint32_t a_counter{};
    while (c++ < 200) {

        // auto &task = tasks.emplace_back([&a_counter](){
        //     printf("%d : hello task, on thread %d\n", a_counter.fetch_add(1u, std::memory_order_relaxed), threads::for_each_thread::worker->id());
        //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // });
        // tp.run(&task);

        auto * task = tp.createTask([&a_counter](){
            printf("%d : hello task, on thread %lu\n", a_counter++, threads::for_each_thread::worker->id());
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
        });
        tp.run(task);
    }

 
    static threads::WaitSource w8;
    {
        threads::StopSource2 stop;
        tp.execute(std::bind([](threads::StopToken token, threads::WaitToken wToken) {
            if (!token.stop_requested()) {
                printf("stop_token task example! on thread %lu\n", threads::for_each_thread::worker->id());
            } else {
               printf("stop_token task example: stop requested! on thread %lu\n", threads::for_each_thread::worker->id()); 
            }

            wToken.request_complete();

        }, stop.get_token(), w8.get_token()));

        //tp.stop();
        // w8.request_wait();
        //stop.request_stop();
    }

    w8.request_wait();
    //tp.stop();
    return 1;

    printf("experemental\n");
    
// auto Cls0 = [](){
//     int a = 0;
// };

// auto sz0 = sizeof(Cls0);

// A aaa;

// auto Cls1 = [aaa](){
//     int a = 0;
// };

// auto sz1 = sizeof(Cls1);

// std::move_only_function<void()> ff = [aaa, aa = aaa, ab = aaa, ad = aaa, ae = aaa](){
//     int a = 0;
// };

// std::function<void()> ff2 = [aaa, aa = aaa, ab = aaa, ad = aaa, ae = aaa](){
//     int a = 0;
// };

// auto sz2 = sizeof(ff);
// auto sz3 = sizeof(ff2);

// int a = 0;


    if (false) {
        threads::ExecutorImpl w([](threads::Executor & w, float delta) {
            w.performTasks();
            static int a = 0;
            std::printf("dt: %f, a = %d\n", delta, ++a);
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }, 0.1f);
        w.run();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        {
            threads::WorkerTasks tasks;
            tasks.reserve(10);
            for (int32_t i = 0; i < 10; ++i) {
                tasks.emplace_back([i](){
                    printf("task %d\n", i);
                });
            }
            w.emplaceTasks(std::move(tasks));
        }


        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        w.pause();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        {
            threads::WorkerTasks tasks;
            tasks.reserve(10);
            for (int32_t i = 0; i < 10; ++i) {
                tasks.emplace_back([i](){
                    printf("task %d\n", i + 100);
                });
            }
            w.emplaceTasks(std::move(tasks));
        }

        w.resume();
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }

    if (false) {
        threads::Worker w;
        w.run();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printf("begin job\n");
        {
            threads::WorkerTasks tasks = stl::make_vector(threads::WorkerTask{[](){
                printf("worker job 0\n");
            }});
            w.emplaceTasks(std::move(tasks));
        }
        {
            threads::WorkerTasks tasks;
            tasks.emplace_back(
                [](){ printf("worker job 1\n"); }
            );
            w.emplaceTasks(std::move(tasks));
        }

        {
            threads::WorkerLoopTasks tasks;
            tasks.emplace_back(1.3f, [](float delta){
                printf("worker loop task2 %f\n", delta);
            });
            tasks.emplace_back(0.5f, [](float delta){
                printf("worker loop task1 %f\n", delta);
            });
            w.emplaceLoopTasks(std::move(tasks));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        w.pause();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (true) {
        std::stop_source stop;
        //std::stop_token token;
        //token = stop.get_token();
        std::thread th([](std::stop_token token){
            while(!token.stop_requested()) {
                printf("step\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }, stop.get_token());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop.request_stop();
        stop.request_stop();
        printf("stop\n");
        th.join();

        //////////////////////////////////
        // struct Task {
        //     std::move_only_function<void()> task;
        //     std::stop_token stopToken;
        // };

        // std::stop_source stop2;
        // Task t {std::move_only_function<void()>([](){
        //     static int i = 0;
        //     printf("!!!!%d\n", ++i);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // }), stop2.get_token()};

        // std::thread th2([](Task && task){
        //     while(!task.stopToken.stop_requested()) {
        //         task.task();
        //     }
        // }, std::move(t));

        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // stop2.request_stop();
        // th2.join();
        //////////////////
        std::stop_source stop2;

        threads::WorkerTasks tasks;
        tasks.emplace_back(
            [stopToken = stop2.get_token()]() {
                if (!stopToken.stop_requested()) {
                    printf("worker job 1\n");
                }
            }
        );

        std::thread th2([stopToken = stop2.get_token()]() {
            while(!stopToken.stop_requested()) {
                static int i = 0;
                printf("!!!!%d\n", i++);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop2.request_stop();
        th2.join();
        //////////////////////
        std::stop_source stop3;
        threads::WorkerTask f = std::bind([](std::stop_token token) {
            while(!token.stop_requested()) {
                static int i = 0;
                printf("____%d\n", i++);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        }, stop3.get_token());

        auto f2 = threads::makeTask([](std::stop_token token) {}, stop3);

        auto f4 = std::bind([](float delta, std::stop_token token) {
            printf("%f\n", delta);
        }, std::placeholders::_1, stop3.get_token());
        f4(25.0f);

        auto f5 = threads::makeLoopTask(1.0f, [](float delta, std::stop_token token) {
            printf("%f\n", delta);
        }, stop3);
        f5.task(10.0f);

        std::thread th3(std::move(f));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop3.request_stop();
        th3.join();
    }

    return 1;
}