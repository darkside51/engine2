#include "utils/profile/leaks.h"

/////////
#include "threads/executor.h"
#include "device/glfw/device.h"
#include "example.h"
#include "utils/json.h"

int main() {
    engine::GLFWModule glfwModule;
    auto & window0 = glfwModule.createWindow("vulkan", 1024u, 768u);
    window0.setVisible(false);

    example::Example example;
    example.createApi();
    if (example.createSurface(window0.getSurfaceCreateInfo())) {
        example.createDevice();
        example.createSwapchain();

        auto const [width, height] = window0.getSize();
        example.resize(width, height);

        example.createRenderPasses();
        
        example.sizeChanged();

        example.createPipelines();

        constexpr float inputCheckTime = 1.0f / 60.0f;
        constexpr float redrawTime = 1.0f / 120.0f;

        threads::Executor renderThread([&](auto & executionContext, float delta) {
            executionContext.performTasks();
            example.update(delta);
            example.draw(delta);
        }, redrawTime);

        renderThread.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            example.prepare();
        }}));

        example.setRenderThread(&renderThread);

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
            renderThread.pause();
            example.resize(w, h);
            example.sizeChanged();
            renderThread.resume();
        });

        window0.setOnClose([&]() {
            renderThread.stop();
            example.destroy();
        });

        while (glfwModule.process()) {
            example.processLogic();
            std::this_thread::sleep_for(std::chrono::duration<float>(inputCheckTime));
        }
        
    }

    return 1;
}