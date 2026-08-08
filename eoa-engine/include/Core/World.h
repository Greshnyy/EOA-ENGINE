#pragma once

#include "Core/Types.h"
#include "Core/Events.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <functional>

namespace EOA {

// ============================================
// КОМПОНЕНТ (в стиле UE: UActorComponent)
// ============================================

class Actor;

class Component {
public:
    virtual ~Component() = default;
    
    Actor* GetOwner() const { return Owner; }
    void SetOwner(Actor* owner) { Owner = owner; }
    
    virtual void Initialize() {}
    virtual void Update(float deltaTime) {}
    virtual void Render() {}
    
    bool IsActive() const { return Active; }
    void SetActive(bool active) { Active = active; }
    
protected:
    Actor* Owner = nullptr;
    bool Active = true;
};

// ============================================
// АКТОР (в стиле UE: AActor)
// ============================================

class Actor {
public:
    virtual ~Actor();
    
    EntityID GetID() const { return ID; }
    const std::string& GetName() const { return Name; }
    void SetName(const std::string& name) { Name = name; }
    
    // Трансформация
    const Vector3& GetPosition() const { return Position; }
    const Vector3& GetRotation() const { return Rotation; }
    const Vector3& GetScale() const { return Scale; }
    
    void SetPosition(const Vector3& pos) { Position = pos; }
    void SetRotation(const Vector3& rot) { Rotation = rot; }
    void SetScale(const Vector3& scale) { Scale = scale; }
    
    Matrix4 GetTransformMatrix() const;
    Matrix4 GetInverseTransformMatrix() const;
    
    // Компоненты
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->SetOwner(this);
        component->Initialize();
        
        T* ptr = component.get();
        Components.push_back(std::move(component));
        ComponentMap[std::type_index(typeid(T))] = ptr;
        
        return ptr;
    }
    
    template<typename T>
    T* GetComponent() {
        auto it = ComponentMap.find(std::type_index(typeid(T)));
        if (it != ComponentMap.end()) {
            return static_cast<T*>(it->second);
        }
        return nullptr;
    }
    
    template<typename T>
    bool HasComponent() {
        return ComponentMap.find(std::type_index(typeid(T))) != ComponentMap.end();
    }
    
    // Обновление
    virtual void Update(float deltaTime);
    virtual void Render();
    
protected:
    friend class World;
    
    Actor() : ID(GenerateID()) {}
    
    static EntityID GenerateID() {
        static EntityID nextID = 1;
        return nextID++;
    }
    
    EntityID ID;
    std::string Name;
    
    Vector3 Position{0.0f, 0.0f, 0.0f};
    Vector3 Rotation{0.0f, 0.0f, 0.0f};
    Vector3 Scale{1.0f, 1.0f, 1.0f};
    
    std::vector<std::unique_ptr<Component>> Components;
    std::unordered_map<std::type_index, Component*> ComponentMap;
};

// ============================================
// МИР (в стиле UE: UWorld)
// ============================================

class World {
public:
    static World* GetInstance() {
        static World instance;
        return &instance;
    }
    
    // Создание акторов
    template<typename T, typename... Args>
    T* SpawnActor(Args&&... args) {
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = actor.get();
        
        Actors.push_back(std::move(actor));
        ActorMap[ptr->GetID()] = ptr;
        
        // Событие спавна
        EventDispatcher* dispatcher = nullptr; // Получить из движка
        // if (dispatcher) dispatcher->Dispatch(ActorSpawnedEvent(ptr));
        
        return ptr;
    }
    
    // Удаление акторов
    void DestroyActor(EntityID id);
    void DestroyActor(Actor* actor);
    
    // Поиск акторов
    Actor* GetActorByID(EntityID id);
    
    template<typename T>
    std::vector<T*> GetActorsByType() {
        std::vector<T*> result;
        for (auto& actor : Actors) {
            if (T* a = dynamic_cast<T*>(actor.get())) {
                result.push_back(a);
            }
        }
        return result;
    }
    
    // Обновление мира
    void Update(float deltaTime);
    void Render();
    
    // Очистка
    void Clear();
    
    size_t GetActorCount() const { return Actors.size(); }
    
private:
    World() = default;
    
    std::vector<std::unique_ptr<Actor>> Actors;
    std::unordered_map<EntityID, Actor*> ActorMap;
};

#define gWorld EOA::World::GetInstance()

} // namespace EOA
