#pragma once
#include "core/object.h"
#include "core/component.h"
#include <vector>
#include <memory>
#include <type_traits>

namespace eoa {

class Actor : public Object {
public:
    EOA_CLASS_DECL(Actor, Object)

    explicit Actor(const std::string& name = "Actor");
    virtual ~Actor() = default;

    virtual void BeginPlay();
    virtual void Tick(float deltaTime);
    virtual void EndPlay();

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = comp.get();
        comp->SetOwner(this);
        components_.push_back(std::move(comp));
        return raw;
    }

    template<typename T>
    T* GetComponent() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        for (auto& c : components_) {
            T* casted = dynamic_cast<T*>(c.get());
            if (casted) return casted;
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        std::vector<T*> result;
        for (auto& c : components_) {
            T* casted = dynamic_cast<T*>(c.get());
            if (casted) result.push_back(casted);
        }
        return result;
    }

    const std::vector<std::unique_ptr<Component>>& GetAllComponents() const {
        return components_;
    }

    bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }

protected:
    std::vector<std::unique_ptr<Component>> components_;
    bool hasBegunPlay_ = false;
    bool visible_ = true;
};

} // namespace eoa
