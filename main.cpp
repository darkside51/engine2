#include "utils/profile/leaks.h"

/////////
#include "threads/executor.h"
#include "device/glfw/device.h"
#include "log/log.h"
#include "example.h"
#include "utils/json.h"

int main() {
    engine::GLFWModule glfwModule;
    #ifdef DEBUG
    auto & window0 = glfwModule.createWindow("vulkan_debug", 1024u, 768u);
    #else
    auto & window0 = glfwModule.createWindow("vulkan_release", 1024u, 768u);
    #endif
    window0.setVisible(false);

    {
        example::Example example;
        example.createApi();
        if (example.createSurface(window0.getSurfaceCreateInfo())) {
            example.createDevice();
            example.createSwapchain();

            example.createRenderPasses();
            example.createPipelines();

            auto const [width, height] = window0.getSize();
            example.resize(width, height);

            constexpr float inputCheckTime = 1.0f / 60.0f;
            constexpr float redrawTime = 1.0f / 120.0f;

            threads::Executor renderThread([&](auto & executionContext, threads::ExecutionTimePoint now, float delta) {
                executionContext.performTasks();
                example.update(now, delta);
                example.draw(now, delta);
            }, redrawTime);

            example.setRenderThread(&renderThread);

            renderThread.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
                example.prepare();
            }}));

            threads::ExecutionContext mainThreadContext;
            mainThreadContext.identifyThreadId();

            example.setMainThread(&mainThreadContext);

            renderThread.run();

            window0.setVisible(true);

            window0.setOnPointer([&](engine::PointerEvent && event) {
                example.onPointerEvent(std::move(event));
            });

            window0.setOnWheel([&](float dx, float dy) {
                example.onWheelEvent(dx, dy);
            });

            window0.setOnKey([&](engine::KeyEvent && event, uint8_t mask) {
                if (event.key == engine::KeyboardKey::eEscape) {
                    window0.close();
                    return;
                }
                example.onKeyEvent(std::move(event), mask);
            });

            window0.setOnResize([&](uint16_t w, uint16_t h) {
                renderThread.emplaceTasks(stl::make_vector(threads::WorkerTask{[&example, w, h]() mutable {
                    example.resize(w, h);
                }}));
                // renderThread.pause();
                // example.resize(w, h);
                // renderThread.resume();
            });

            window0.setOnClose([&]() {
                renderThread.stop();
                example.destroy();
            });

            // uint32_t counter = 0u;
            while (glfwModule.process()) {
                // LOG_I("%d", counter);
                // if (++counter == 100) {
                //     window0.close();
                //     continue;
                // }
                mainThreadContext.performTasks();
                example.deviceProcess();
                std::this_thread::sleep_for(std::chrono::duration<float>(inputCheckTime));
            }    
        }
    }

    LOG_I("application finished!");

    return 1;
}