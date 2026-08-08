#include "platform/camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace eoa {

Camera::Camera(glm::vec3 position, float yawDegrees, float pitchDegrees)
    : position_(position), yaw_(yawDegrees), pitch_(pitchDegrees) {}

glm::vec3 Camera::Forward() const {
    float yawRad = glm::radians(yaw_);
    float pitchRad = glm::radians(pitch_);
    return glm::normalize(glm::vec3(
        std::cos(pitchRad) * std::sin(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::cos(yawRad)));
}

glm::vec3 Camera::Right() const {
    return glm::normalize(glm::cross(Forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Camera::Update(const Window& window, float deltaTime) {
    GLFWwindow* handle = window.Handle();

    bool rmbDown = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    double mx, my;
    glfwGetCursorPos(handle, &mx, &my);

    if (rmbDown && !wasRmbDown_) {
        // ПКМ только что зажали — захватываем курсор, запоминаем позицию,
        // чтобы не дёрнуть камеру первым же кадром на дельту от старой позиции.
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        lastMouseX_ = mx;
        lastMouseY_ = my;
    } else if (!rmbDown && wasRmbDown_) {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (rmbDown) {
        double dx = mx - lastMouseX_;
        double dy = my - lastMouseY_;
        yaw_ -= static_cast<float>(dx) * mouseSensitivity_;
        pitch_ -= static_cast<float>(dy) * mouseSensitivity_; // инверсия: вверх мышью = вверх взгляд
        pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    }
    lastMouseX_ = mx;
    lastMouseY_ = my;
    wasRmbDown_ = rmbDown;

    glm::vec3 forward = Forward();
    glm::vec3 right = Right();
    float velocity = moveSpeed_ * deltaTime;

    if (glfwGetKey(handle, GLFW_KEY_W) == GLFW_PRESS) position_ += forward * velocity;
    if (glfwGetKey(handle, GLFW_KEY_S) == GLFW_PRESS) position_ -= forward * velocity;
    if (glfwGetKey(handle, GLFW_KEY_D) == GLFW_PRESS) position_ += right * velocity;
    if (glfwGetKey(handle, GLFW_KEY_A) == GLFW_PRESS) position_ -= right * velocity;
    if (glfwGetKey(handle, GLFW_KEY_SPACE) == GLFW_PRESS)
        position_ += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    if (glfwGetKey(handle, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        position_ -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
}

glm::mat4 Camera::ViewMatrix() const {
    return glm::lookAt(position_, position_ + Forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace eoa
