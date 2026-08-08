#include "platform/window.h"
#include "log.h"
#include <stdexcept>

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

    // Устанавливаем указатель на this для callback'ов
    glfwSetWindowUserPointer(handle_, this);

    // Регистрируем callback'и
    glfwSetKeyCallback(handle_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_KeyCallback) {
            self->m_KeyCallback(key, action, mods);
        }
    });

    glfwSetCursorPosCallback(handle_, [](GLFWwindow* window, double xpos, double ypos) {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_MouseMoveCallback) {
            self->m_MouseMoveCallback(xpos, ypos);
        }
    });

    glfwSetScrollCallback(handle_, [](GLFWwindow* window, double xoffset, double yoffset) {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_MouseScrollCallback) {
            self->m_MouseScrollCallback(xoffset, yoffset);
        }
    });

    glfwSetMouseButtonCallback(handle_, [](GLFWwindow* window, int button, int action, int mods) {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self && self->m_MouseButtonCallback) {
            self->m_MouseButtonCallback(button, action, mods);
        }
    });

    // Callback на изменение размера
    glfwSetFramebufferSizeCallback(handle_, [](GLFWwindow* window, int w, int h) {
        Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self) {
            self->width_ = w;
            self->height_ = h;
        }
    });

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
