#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

namespace eoa {

// ============================================================================
// КОМПОНЕНТ (базовый класс)
// ============================================================================

class Actor;

class Component {
public:
    virtual ~Component() = default;
    
    Actor* GetOwner() const { return owner_; }
    const std::string& GetName() const { return name_; }
    bool IsActive() const { return active_; }
    
    void SetActive(bool active) { active_ = active; }
    
    // Жизненный цикл
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(double dt) {}
    virtual void OnFixedUpdate(double dt) {}
    virtual void OnRender() {}

protected:
    friend class Actor;
    
    std::string name_;
    Actor* owner_ = nullptr;
    bool active_ = true;
    
    void SetOwner(Actor* owner) { owner_ = owner; }
};

// ============================================================================
// АКТОР (базовый класс)
// ============================================================================

using ComponentPtr = std::shared_ptr<Component>;

class Actor {
public:
    Actor(const std::string& name = "Actor") 
        : name_(name), id_(GenerateId()) {}
    
    virtual ~Actor() {
        // Уничтожение компонентов
        for (auto& comp : components_) {
            comp->OnDestroy();
        }
    }
    
    uint64_t GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    
    bool IsActive() const { return active_; }
    void SetActive(bool active) { active_ = active; }
    
    // Добавление компонента
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must be a Component");
        
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->SetOwner(this);
        comp->name_ = typeid(T).name();
        components_.push_back(comp);
        comp->OnCreate();
        
        // Событие
        // EventSystem::Send<ComponentAddedEvent>(this, comp.get());
        
        return comp.get();
    }
    
    // Получение компонента
    template<typename T>
    T* GetComponent() {
        for (auto& comp : components_) {
            if (auto typed = std::dynamic_pointer_cast<T>(comp)) {
                return typed.get();
            }
        }
        return nullptr;
    }
    
    template<typename T>
    const T* GetComponent() const {
        for (const auto& comp : components_) {
            if (auto typed = std::dynamic_pointer_cast<T>(comp)) {
                return typed.get();
            }
        }
        return nullptr;
    }
    
    // Проверка наличия компонента
    template<typename T>
    bool HasComponent() const {
        return GetComponent<T>() != nullptr;
    }
    
    // Удаление компонента
    template<typename T>
    void RemoveComponent() {
        auto it = std::find_if(components_.begin(), components_.end(),
            [](const ComponentPtr& comp) {
                return dynamic_cast<T*>(comp.get()) != nullptr;
            });
        
        if (it != components_.end()) {
            (*it)->OnDestroy();
            // EventSystem::Send<ComponentRemovedEvent>(this, it->get());
            components_.erase(it);
        }
    }
    
    // Обновление всех компонентов
    void UpdateComponents(double dt) {
        if (!active_) return;
        
        for (auto& comp : components_) {
            if (comp->IsActive()) {
                comp->OnUpdate(dt);
            }
        }
    }
    
    void FixedUpdateComponents(double dt) {
        if (!active_) return;
        
        for (auto& comp : components_) {
            if (comp->IsActive()) {
                comp->OnFixedUpdate(dt);
            }
        }
    }
    
    void RenderComponents() {
        if (!active_) return;
        
        for (auto& comp : components_) {
            if (comp->IsActive()) {
                comp->OnRender();
            }
        }
    }
    
    // Все компоненты
    const std::vector<ComponentPtr>& GetComponents() const {
        return components_;
    }

protected:
    std::string name_;
    uint64_t id_;
    std::vector<ComponentPtr> components_;
    bool active_ = true;
    
private:
    static uint64_t GenerateId() {
        static uint64_t nextId = 0;
        return ++nextId;
    }
};

// ============================================================================
// МИР (WORLD)
// ============================================================================

using ActorPtr = std::shared_ptr<Actor>;

class World {
public:
    World(const std::string& name = "World") : name_(name) {}
    
    const std::string& GetName() const { return name_; }
    
    // Создание актора
    template<typename T = Actor>
    T* CreateActor(const std::string& name = "") {
        std::string actorName = name.empty() ? typeid(T).name() : name;
        auto actor = std::make_shared<T>(actorName);
        actors_.push_back(actor);
        
        // Событие
        // EventSystem::Send<ActorSpawnedEvent>(actor.get());
        
        return actor.get();
    }
    
    // Уничтожение актора
    void DestroyActor(Actor* actor) {
        if (!actor) return;
        
        uint64_t id = actor->GetId();
        
        auto it = std::find_if(actors_.begin(), actors_.end(),
            [actor](const ActorPtr& ptr) {
                return ptr.get() == actor;
            });
        
        if (it != actors_.end()) {
            // Событие
            // EventSystem::Send<ActorDestroyedEvent>(id);
            
            actors_.erase(it);
            pendingDestroy_.push_back(id);
        }
    }
    
    // Уничтожение отложенных акторов
    void ProcessPendingDestroy() {
        pendingDestroy_.clear();
    }
    
    // Поиск актора по ID
    Actor* FindActorById(uint64_t id) {
        for (auto& actor : actors_) {
            if (actor->GetId() == id) {
                return actor.get();
            }
        }
        return nullptr;
    }
    
    // Поиск актора по имени
    Actor* FindActorByName(const std::string& name) {
        for (auto& actor : actors_) {
            if (actor->GetName() == name) {
                return actor.get();
            }
        }
        return nullptr;
    }
    
    // Поиск по тегу
    std::vector<Actor*> FindActorsByTag(const std::string& tag) {
        std::vector<Actor*> result;
        for (auto& actor : actors_) {
            // TODO: Проверка тега
            result.push_back(actor.get());
        }
        return result;
    }
    
    // Все акторы
    const std::vector<ActorPtr>& GetActors() const {
        return actors_;
    }
    
    size_t GetActorCount() const {
        return actors_.size();
    }
    
    // Очистка мира
    void Clear() {
        actors_.clear();
        pendingDestroy_.clear();
    }
    
    // Обновление всех акторов
    void UpdateActors(double dt) {
        for (auto& actor : actors_) {
            actor->UpdateComponents(dt);
        }
    }
    
    void FixedUpdateActors(double dt) {
        for (auto& actor : actors_) {
            actor->FixedUpdateComponents(dt);
        }
    }
    
    void RenderActors() {
        for (auto& actor : actors_) {
            actor->RenderComponents();
        }
    }

private:
    std::string name_;
    std::vector<ActorPtr> actors_;
    std::vector<uint64_t> pendingDestroy_;
};

} // namespace eoa
