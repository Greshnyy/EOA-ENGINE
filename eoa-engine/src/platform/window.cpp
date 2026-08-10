#include "platform/window.h"
#include "log.h"
#include <stdexcept>

namespace eoa {

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height) {
    if (!glfwInit()) EOA_FATAL("glfwInit() failed — платформа без окна/дисплея?");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    handle_ = glfwCreateWindow(width_, height_, title.c_str(), nullptr, nullptr);
    if (!handle_) EOA_FATAL("glfwCreateWindow() failed");
    glfwSetWindowUserPointer(handle_, this);

    glfwSetKeyCallback(handle_, [](GLFWwindow* window, int key, int, int action, int mods) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_KeyCallback) self->m_KeyCallback(key, action, mods);
    });
    glfwSetCursorPosCallback(handle_, [](GLFWwindow* window, double xpos, double ypos) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_MouseMoveCallback) self->m_MouseMoveCallback(xpos, ypos);
    });
    glfwSetScrollCallback(handle_, [](GLFWwindow* window, double xoffset, double yoffset) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_MouseScrollCallback) self->m_MouseScrollCallback(xoffset, yoffset);
    });
    glfwSetMouseButtonCallback(handle_, [](GLFWwindow* window, int button, int action, int mods) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_MouseButtonCallback) self->m_MouseButtonCallback(button, action, mods);
    });
    glfwSetFramebufferSizeCallback(handle_, [](GLFWwindow* window, int w, int h) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!self) return;
        self->width_ = w;
        self->height_ = h;
        if (self->m_FramebufferResizeCallback) self->m_FramebufferResizeCallback(w, h);
    });

    EOA_LOG("Window created: %dx%d \"%s\"", width_, height_, title.c_str());
}

Window::~Window() {
    if (handle_) glfwDestroyWindow(handle_);
    glfwTerminate();
}

bool Window::ShouldClose() const { return handle_ && glfwWindowShouldClose(handle_); }
void Window::PollEvents() const { glfwPollEvents(); }

void Window::FramebufferSize(int& outWidth, int& outHeight) const {
    if (!handle_) { outWidth = outHeight = 0; return; }
    glfwGetFramebufferSize(handle_, &outWidth, &outHeight);
}

void Window::WaitWhileMinimized() const {
    int w=0,h=0;
    while (handle_) {
        glfwGetFramebufferSize(handle_, &w, &h);
        if (w > 0 && h > 0) break;
        glfwWaitEvents();
    }
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    EOA_CHECK_VK(glfwCreateWindowSurface(instance, handle_, nullptr, &surface));
    return surface;
}

} // namespace eoa
