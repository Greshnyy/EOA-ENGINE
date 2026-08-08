#include "core/transform_component.h"
#include "reflection/eoa_reflection.h" // Добавьте этот инклуд

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

    auto info = std::make_unique<ClassInfo>("TransformComponent", []() -> Object* { return new TransformComponent(); });
    
    // Используем правильный синтаксис MakeProperty
    info->AddProperty(MakeProperty<TransformComponent>(
        "Position", PropertyType::Vec3, &TransformComponent::position_
    ));
    info->AddProperty(MakeProperty<TransformComponent>(
        "Rotation", PropertyType::Quat, &TransformComponent::rotation_
    ));
    info->AddProperty(MakeProperty<TransformComponent>(
        "Scale", PropertyType::Vec3, &TransformComponent::scale_
    ));

    ReflectionManager::Get().RegisterClass(std::move(info));
    registered = true;
}

} // namespace eoa
