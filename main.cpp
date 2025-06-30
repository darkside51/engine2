#include "utils/profile/leaks.h"
#include "tests/all_tests.h"

/////////
#include "threads/executor.h"
#include "device/glfw/device.h"
#include "example.h"
#include "utils/json.h"

int main() {
    engine::GLFWModule glfwModule;
    auto & window0 = glfwModule.createWindow("vulkan", 1024u, 768u);
    window0.setVisible(false);

    example::Example ex;
    ex.createApi();
    if (ex.createSurface(window0.getSurfaceCreateInfo())) {
        ex.createDevice();
        ex.createSwapchain();

        auto const [width, height] = window0.getSize();
        ex.resize(width, height);

        ex.createRenderPasses();
        
        ex.sizeChanged();

        ex.createPipelines();

        constexpr float inputCheckTime = 1.0f / 60.0f;
        constexpr float redrawTime = 1.0f / 120.0f;

        threads::Executor executor([&](auto & executionContext, float delta) {
            executionContext.performTasks();
            ex.update(delta);
            ex.draw(delta);
        }, redrawTime);

        executor.emplaceTasks(stl::make_vector(threads::WorkerTask{[&](){
            ex.prepare();
        }}));

        executor.run();

        window0.setVisible(true);

        window0.setOnPointer([&](engine::PointerEvent && event) {
            ex.onPointerEvent(std::move(event));
        });

        window0.setOnWheel([&](float dx, float dy) {
            ex.onWheelEvent(dx, dy);
        });

        window0.setOnKey([&](engine::KeyEvent && event, uint8_t mask) {
            if (event.key == engine::KeyboardKey::eEscape) {
                window0.close();
                return;
            }
            ex.onKeyEvent(std::move(event), mask);
        });

        window0.setOnResize([&](uint16_t w, uint16_t h) {
            executor.pause();
            ex.resize(w, h);
            ex.sizeChanged();
            executor.resume();
        });

        window0.setOnClose([&]() {
            executor.stop();
            ex.destroy();
        });

        while (glfwModule.process()) {
            std::this_thread::sleep_for(std::chrono::duration<float>(inputCheckTime));
        }
        
    }

    return 1;
}