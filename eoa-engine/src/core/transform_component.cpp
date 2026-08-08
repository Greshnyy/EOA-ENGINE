#include "core/transform_component.h"

namespace eoa {

TransformComponent::TransformComponent(const std::string& name) {
    SetName(name);
}

glm::mat4 TransformComponent::GetModelMatrix() {
    if (dirty_) {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position_);
        m = m * glm::mat4_cast(rotation_);
        m = glm::scale(m, scale_);
        cachedMatrix_ = m;
        dirty_ = false;
    }
    return cachedMatrix_;
}

glm::vec3 TransformComponent::Forward() const {
    return glm::normalize(rotation_ * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 TransformComponent::Right() const {
    return glm::normalize(rotation_ * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 TransformComponent::Up() const {
    return glm::normalize(rotation_ * glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace eoa
