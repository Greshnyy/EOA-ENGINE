#include "core/actor.h"

namespace eoa {

Actor::Actor(const std::string& name) {
    SetName(name);
}

void Actor::BeginPlay() {
    for (auto& comp : components_) {
        comp->BeginPlay();
    }
    hasBegunPlay_ = true;
}

void Actor::Tick(float deltaTime) {
    for (auto& comp : components_) {
        if (comp->IsActive()) {
            comp->Tick(deltaTime);
        }
    }
}

void Actor::EndPlay() {
    for (auto& comp : components_) {
        comp->EndPlay();
    }
}

// Регистрация класса Actor
EOA_CLASS_IMPL(Actor, Object)
EOA_END_CLASS_IMPL()

} // namespace eoa
