#include "core/transform_component.h"

#include <glm/gtx/quaternion.hpp>

namespace eoa {

TransformComponent::TransformComponent(const std::string& name)
    : Component(name) {
}

glm::mat4 TransformComponent::GetModelMatrix() {
    if (dirty_) {
        cachedMatrix_ = glm::translate(glm::mat4(1.0f), position_)
                      * glm::toMat4(rotation_)
                      * glm::scale(glm::mat4(1.0f), scale_);
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
