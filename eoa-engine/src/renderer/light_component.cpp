#include "renderer/light_component.h"
#include "core/actor.h"
#include "core/transform_component.h"

namespace eoa {

LightComponent::LightComponent(const std::string& name) {
    SetName(name);
}

void LightComponent::FillShaderData(ShaderData& data) const {
    data.color = color_;
    data.intensity = intensity_;

    glm::vec3 pos(0.0f);
    glm::vec3 dir(0.0f, -1.0f, 0.0f);

    if (owner_) {
        auto* transform = owner_->GetComponent<TransformComponent>();
        if (transform) {
            pos = transform->GetPosition();
            dir = transform->Forward();
        }
    }

    data.position = pos;
    data.direction = dir;
    data.type = static_cast<uint32_t>(type_);
    data.range = range_;
    data.innerCone = glm::cos(glm::radians(innerConeAngle_));
    data.outerCone = glm::cos(glm::radians(outerConeAngle_));
}

} // namespace eoa
