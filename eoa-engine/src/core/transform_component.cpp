#include "core/transform_component.h"
#include "core/type_info.h"

namespace eoa {

TransformComponent::TransformComponent()
    : position_(0.0f), rotation_(1.0f, 0.0f, 0.0f, 0.0f), scale_(1.0f) {
}

void TransformComponent::OnUpdate(float deltaTime) {
    // Логика обновления
}

// Функция регистрации класса
void TransformComponent::RegisterReflection() {
    static bool registered = false;
    if (registered) return;

    auto cls = std::make_unique<Class>("TransformComponent", "Component");
    cls->SetSize(sizeof(TransformComponent));
    cls->SetConstructor([]() -> std::unique_ptr<Object> {
        return std::make_unique<TransformComponent>();
    });
    
    cls->AddProperty(MakeProperty<TransformComponent>(
        "Position", PropertyType::Vec3, &TransformComponent::position_
    ));
    cls->AddProperty(MakeProperty<TransformComponent>(
        "Rotation", PropertyType::Quat, &TransformComponent::rotation_
    ));
    cls->AddProperty(MakeProperty<TransformComponent>(
        "Scale", PropertyType::Vec3, &TransformComponent::scale_
    ));

    ReflectionSystem::Get().RegisterClass(std::move(cls));
    registered = true;
}

} // namespace eoa
