#pragma once
#include <glm/glm.hpp>
#include "platform/window.h"

namespace eoa {

// Свободная FPS-камера: WASD — движение, зажатая ПКМ + мышь — обзор.
// Курсор захватывается только пока зажата ПКМ (не блокирует мышь на весь
// экран постоянно — это раздражает при отладке/переключении окон).
class Camera {
public:
    Camera(glm::vec3 position, float yawDegrees, float pitchDegrees);

    // deltaTime в секундах. Читает состояние клавиатуры/мыши из окна сама.
    void Update(const Window& window, float deltaTime);

    glm::mat4 ViewMatrix() const;
    glm::vec3 Position() const { return position_; }

private:
    glm::vec3 position_;
    float yaw_;   // градусы, вокруг Y
    float pitch_; // градусы, вокруг X, зажат [-89, 89]

    bool wasRmbDown_ = false;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;

    float moveSpeed_ = 3.0f;      // единиц/сек — под масштаб тестовой сцены
    float mouseSensitivity_ = 0.12f;

    glm::vec3 Forward() const;
    glm::vec3 Right() const;
};

} // namespace eoa
