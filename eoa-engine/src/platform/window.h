#pragma once
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace eoa {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void PollEvents() const;
    VkSurfaceKHR CreateSurface(VkInstance instance) const;
    GLFWwindow* Handle() const { return handle_; }
    int Width() const { return width_; }
    int Height() const { return height_; }
    void FramebufferSize(int& outWidth, int& outHeight) const;
    void WaitWhileMinimized() const;

    using KeyCallback = std::function<void(int, int, int)>;
    using MouseMoveCallback = std::function<void(double, double)>;
    using MouseScrollCallback = std::function<void(double, double)>;
    using MouseButtonCallback = std::function<void(int, int, int)>;
    using FramebufferResizeCallback = std::function<void(int, int)>;

    void SetKeyCallback(KeyCallback callback) { m_KeyCallback = std::move(callback); }
    void SetMouseMoveCallback(MouseMoveCallback callback) { m_MouseMoveCallback = std::move(callback); }
    void SetMouseScrollCallback(MouseScrollCallback callback) { m_MouseScrollCallback = std::move(callback); }
    void SetMouseButtonCallback(MouseButtonCallback callback) { m_MouseButtonCallback = std::move(callback); }
    void SetFramebufferResizeCallback(FramebufferResizeCallback callback) { m_FramebufferResizeCallback = std::move(callback); }

private:
    GLFWwindow* handle_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    KeyCallback m_KeyCallback;
    MouseMoveCallback m_MouseMoveCallback;
    MouseScrollCallback m_MouseScrollCallback;
    MouseButtonCallback m_MouseButtonCallback;
    FramebufferResizeCallback m_FramebufferResizeCallback;
};

} // namespace eoa
