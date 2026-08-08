#include "renderer/camera_component.h"
#include "core/actor.h"
#include "core/transform_component.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace eoa {

CameraComponent::CameraComponent(const std::string& name) {
    SetName(name);
}

void CameraComponent::SetPerspective(float fovDeg, float aspect,
                                      float nearPlane, float farPlane) {
    projMatrix_ = glm::perspective(glm::radians(fovDeg), aspect,
                                    nearPlane, farPlane);
    // Vulkan: инвертированная Y (clip space Y-up → Y-down)
    projMatrix_[1][1] *= -1.0f;
}

glm::mat4 CameraComponent::GetViewMatrix() const {
    if (!owner_) return glm::mat4(1.0f);

    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return glm::mat4(1.0f);

    glm::vec3 pos = transform->GetPosition();
    glm::vec3 forward = transform->Forward();

    return glm::lookAt(pos, pos + forward, glm::vec3(0.0f, 1.0f, 0.0f));
}

void CameraComponent::ProcessMovement(float forward, float right, float up,
                                       float deltaTime) {
    if (!owner_) return;

    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return;

    glm::vec3 pos = transform->GetPosition();
    glm::vec3 fwd = transform->Forward();
    glm::vec3 rgt = transform->Right();

    pos += fwd * forward * moveSpeed_ * deltaTime;
    pos += rgt * right * moveSpeed_ * deltaTime;
    pos += glm::vec3(0.0f, 1.0f, 0.0f) * up * moveSpeed_ * deltaTime;

    transform->SetPosition(pos);
}

void CameraComponent::ProcessMouseLook(float deltaYaw, float deltaPitch) {
    yaw_ += deltaYaw * mouseSensitivity_;
    pitch_ += deltaPitch * mouseSensitivity_;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);

    if (!owner_) return;
    auto* transform = owner_->GetComponent<TransformComponent>();
    if (!transform) return;

    glm::quat qYaw = glm::angleAxis(glm::radians(-yaw_), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat qPitch = glm::angleAxis(glm::radians(-pitch_), glm::vec3(1.0f, 0.0f, 0.0f));
    transform->SetRotation(qYaw * qPitch);
}

} // namespace eoa
