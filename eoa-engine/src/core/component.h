#pragma once
#include "core/object.h"
#include <memory>

namespace eoa {

class Actor;

class Component : public Object {
public:
    EOA_CLASS_DECL(Component, Object)

    explicit Component(const std::string& name = "Component");
    ~Component() override = default;

    virtual void BeginPlay() {}
    virtual void Tick(float deltaTime) {}
    virtual void EndPlay() {}

    Actor* GetOwner() const { return owner_; }
    void SetOwner(Actor* owner) { owner_ = owner; }

    bool IsActive() const { return active_; }
    void SetActive(bool active) { active_ = active; }

protected:
    Actor* owner_ = nullptr;
    bool active_ = true;
};

} // namespace eoa
