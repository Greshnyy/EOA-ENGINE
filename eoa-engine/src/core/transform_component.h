#pragma once
#include "core/component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace eoa {

class TransformComponent : public Component {
public:
    EOA_CLASS_DECL(TransformComponent, Component)

    explicit TransformComponent(const std::string& name = "Transform");

    const glm::vec3& GetPosition() const { return position_; }
    void SetPosition(const glm::vec3& pos) { position_ = pos; dirty_ = true; }

    const glm::quat& GetRotation() const { return rotation_; }
    void SetRotation(const glm::quat& rot) { rotation_ = rot; dirty_ = true; }

    const glm::vec3& GetScale() const { return scale_; }
    void SetScale(const glm::vec3& scl) { scale_ = scl; dirty_ = true; }

    // Мировая модель-матрица (пересчитывается только при dirty)
    glm::mat4 GetModelMatrix();

    // Направление forward (локальное -Z, как в UE/game-стандартах)
    glm::vec3 Forward() const;
    glm::vec3 Right() const;
    glm::vec3 Up() const;

private:
    glm::vec3 position_ = glm::vec3(0.0f);
    glm::quat rotation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale_ = glm::vec3(1.0f);
    glm::mat4 cachedMatrix_ = glm::mat4(1.0f);
    bool dirty_ = true;
};

} // namespace eoa
