#include "core/transform_component.h"
#include "core/type_info.h"
#include <glm/gtx/quaternion.hpp>

namespace eoa {

TransformComponent::TransformComponent(const std::string& name)
    : Component(name), position_(0.0f), rotation_(1.0f, 0.0f, 0.0f, 0.0f), scale_(1.0f), cachedMatrix_(1.0f), dirty_(true) {}

glm::mat4 TransformComponent::GetModelMatrix() {
    if (dirty_) {
        cachedMatrix_ = glm::translate(glm::mat4(1.0f), position_)
            * glm::toMat4(rotation_)
            * glm::scale(glm::mat4(1.0f), scale_);
        dirty_ = false;
    }
    return cachedMatrix_;
}

glm::vec3 TransformComponent::Forward() const { return rotation_ * glm::vec3(0.0f, 0.0f, -1.0f); }
glm::vec3 TransformComponent::Right() const { return rotation_ * glm::vec3(1.0f, 0.0f, 0.0f); }
glm::vec3 TransformComponent::Up() const { return rotation_ * glm::vec3(0.0f, 1.0f, 0.0f); }

void TransformComponent::RegisterReflection() {
    static bool registered = false;
    if (registered) return;
    auto cls = std::make_unique<Class>("TransformComponent", "Component");
    cls->SetSize(sizeof(TransformComponent));
    cls->SetConstructor([]() -> std::unique_ptr<Object> { return std::make_unique<TransformComponent>(); });
    cls->AddProperty(MakeProperty<TransformComponent>("Position", PropertyType::Vec3, &TransformComponent::position_));
    cls->AddProperty(MakeProperty<TransformComponent>("Rotation", PropertyType::Quat, &TransformComponent::rotation_));
    cls->AddProperty(MakeProperty<TransformComponent>("Scale", PropertyType::Vec3, &TransformComponent::scale_));
    ReflectionSystem::Get().RegisterClass(std::move(cls));
    registered = true;
}

} // namespace eoa
