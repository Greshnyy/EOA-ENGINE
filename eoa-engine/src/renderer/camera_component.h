#pragma once
#include "core/component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eoa {

class CameraComponent : public Component {
public:
    const char* ClassName() const override { return "CameraComponent"; }

    explicit CameraComponent(const std::string& name = "Camera");

    // Проекция
    void SetPerspective(float fovDeg, float aspect, float nearPlane, float farPlane);
    glm::mat4 GetProjectionMatrix() const { return projMatrix_; }

    // Видовая матрица (вычисляется из TransformComponent владельца)
    glm::mat4 GetViewMatrix() const;

    // FPS-управление (перенесено из platform/camera.h)
    void ProcessMovement(float forward, float right, float up, float deltaTime);
    void ProcessMouseLook(float deltaYaw, float deltaPitch);

    // Параметры
    float GetMoveSpeed() const { return moveSpeed_; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
    float GetMouseSensitivity() const { return mouseSensitivity_; }
    void SetMouseSensitivity(float sens) { mouseSensitivity_ = sens; }

    float GetYaw() const { return yaw_; }
    float GetPitch() const { return pitch_; }

private:
    glm::mat4 projMatrix_ = glm::mat4(1.0f);
    float yaw_ = 0.0f;
    float pitch_ = -10.0f;
    float moveSpeed_ = 5.0f;
    float mouseSensitivity_ = 0.15f;
};

} // namespace eoa
