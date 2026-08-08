#include "platform/window.h"
#include "log.h"

namespace eoa {

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height) {
    if (!glfwInit()) {
        EOA_FATAL("glfwInit() failed — платформа без окна/дисплея?");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan, не OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    handle_ = glfwCreateWindow(width_, height_, title.c_str(), nullptr, nullptr);
    if (!handle_) {
        EOA_FATAL("glfwCreateWindow() failed");
    }

    EOA_LOG("Window created: %dx%d \"%s\"", width_, height_, title.c_str());
}

Window::~Window() {
    if (handle_) {
        glfwDestroyWindow(handle_);
    }
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle_);
}

void Window::PollEvents() const {
    glfwPollEvents();
}

void Window::FramebufferSize(int& outWidth, int& outHeight) const {
    glfwGetFramebufferSize(handle_, &outWidth, &outHeight);
}

void Window::WaitWhileMinimized() const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(handle_, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(handle_, &w, &h);
        glfwWaitEvents();
    }
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    EOA_CHECK_VK(glfwCreateWindowSurface(instance, handle_, nullptr, &surface));
    return surface;
}

} // namespace eoa
