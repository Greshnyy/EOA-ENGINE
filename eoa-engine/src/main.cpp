#include "log.h"
#include "platform/window.h"
#include "platform/camera.h"
#include "rhi/vk_instance.h"
#include "rhi/vk_device.h"
#include "renderer/renderer.h"
#include "editor/editor.h"
#include <chrono>

int main() {
    EOA_LOG("EOA Engine starting...");

#ifdef NDEBUG
    constexpr bool kEnableValidation = false;
#else
    constexpr bool kEnableValidation = true;
#endif

    eoa::Window window(1280, 720, "Echoes Engine — Engine");
    eoa::VulkanInstance instance(kEnableValidation);

    VkSurfaceKHR surface = window.CreateSurface(instance.Handle());
    eoa::VulkanDevice device(instance.Handle(), surface);

    int fbWidth = 0, fbHeight = 0;
    window.FramebufferSize(fbWidth, fbHeight);

    eoa::Camera camera(glm::vec3(0.0f, 0.0f, -5.0f), /*yaw*/ 0.0f, /*pitch*/ -10.0f);

    auto lastFrameTime = std::chrono::steady_clock::now();

    {
        eoa::Renderer renderer(device.Physical(), device.Logical(), surface,
                                device.GraphicsQueueFamily(), device.GraphicsQueue(),
                                static_cast<uint32_t>(fbWidth), static_cast<uint32_t>(fbHeight));

        eoa::Editor editor(window.Handle(), instance.Handle(),
                           device.Physical(), device.Logical(),
                           device.GraphicsQueueFamily(), device.GraphicsQueue(),
                           renderer.GetRenderPass(), 3);
        editor.SetWorld(&renderer.GetWorld());
        renderer.SetEditor(&editor);
        editor.SetOnAssetActivated([&renderer](const std::string& path) {
            renderer.LoadGltfIntoWorld(path);
        });

        editor.Log("Engine started — EOA Editor");
        editor.Log("WASD — движение, зажми ПКМ и води мышью — обзор");

        while (!window.ShouldClose()) {
            window.PollEvents();
            window.WaitWhileMinimized();

            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
            lastFrameTime = now;

            editor.NewFrame();

            if (!editor.WantsCaptureMouse() && !editor.WantsCaptureKeyboard()) {
                camera.Update(window, deltaTime);
            }

            window.FramebufferSize(fbWidth, fbHeight);
            renderer.DrawFrame(static_cast<uint32_t>(fbWidth), static_cast<uint32_t>(fbHeight),
                                camera.ViewMatrix());
        }
    } // renderer + editor destroyed here (swapchain freed before surface)

    vkDestroySurfaceKHR(instance.Handle(), surface, nullptr);
    EOA_LOG("Shutdown clean.");
    return 0;
}