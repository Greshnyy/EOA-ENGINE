#pragma once
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace eoa {

// Обёртка над GLFW-окном. Один экземпляр = одно окно приложения.
class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void PollEvents() const;

    // Создаёт VkSurfaceKHR для этого окна в переданном инстансе.
    VkSurfaceKHR CreateSurface(VkInstance instance) const;

    GLFWwindow* Handle() const { return handle_; }
    int Width() const { return width_; }
    int Height() const { return height_; }

    // Актуальный размер framebuffer'а прямо сейчас (не кэш из конструктора) —
    // нужен для resize/минимизации.
    void FramebufferSize(int& outWidth, int& outHeight) const;

    // Блокирует, пока окно свёрнуто (размер 0x0) — ждёт события ресайза.
    void WaitWhileMinimized() const;

private:
    GLFWwindow* handle_ = nullptr;
    int width_;
    int height_;
};

} // namespace eoa
