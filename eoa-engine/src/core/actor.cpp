#include "core/actor.h"

namespace eoa {

Actor::Actor(const std::string& name) {
    SetName(name);
}

void Actor::BeginPlay() {
    if (hasBegunPlay_) {
        return;
    }

    for (auto& comp : components_) {
        if (comp && comp->IsActive()) {
            comp->BeginPlay();
        }
    }

    hasBegunPlay_ = true;
}

void Actor::Tick(float deltaTime) {
    if (!hasBegunPlay_) {
        BeginPlay();
    }

    for (auto& comp : components_) {
        if (comp && comp->IsActive()) {
            comp->Tick(deltaTime);
        }
    }
}

void Actor::EndPlay() {
    if (!hasBegunPlay_) {
        return;
    }

    for (auto& comp : components_) {
        if (comp) {
            comp->EndPlay();
        }
    }

    hasBegunPlay_ = false;
}

EOA_CLASS_IMPL(Actor, Object)
EOA_END_CLASS_IMPL()

} // namespace eoa
