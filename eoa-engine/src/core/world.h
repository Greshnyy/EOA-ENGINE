#pragma once
#include "core/object.h"
#include "core/actor.h"
#include "core/component.h"
#include "core/serializer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace eoa {

// Forward declare Component
class Component;

// ============================================================================
// World - контейнер для всех объектов сцены
// ============================================================================
class World : public Object {
    EOA_CLASS_DECL(World, Object)
    
public:
    World();
    ~World() override;
    
    // Создать новый Actor в мире
    Actor* CreateActor(const std::string& name = "Actor");
    
    // Удалить Actor из мира
    void DestroyActor(Actor* actor);
    
    // Получить все Actors
    const std::vector<Actor*>& GetActors() const { return actors_; }
    std::vector<Actor*>& GetActors() { return actors_; }
    
    // Найти Actor по имени
    Actor* FindActorByName(const std::string& name) const;
    
    // Найти Actor по ID
    Actor* FindActorById(uint64_t id) const;
    
    // Получить количество Actors
    size_t GetActorCount() const { return actors_.size(); }
    
    // Очистить мир (удалить все Actors)
    void Clear();
    
    // Сериализация мира
    std::string Serialize() const;
    bool Deserialize(const std::string& data);
    
    // Сохранить/загрузить мир в файл
    bool SaveToFile(const std::string& filename) const;
    bool LoadFromFile(const std::string& filename);
    
    // Получить главный Camera Actor
    Actor* GetMainCamera() const { return mainCamera_; }
    void SetMainCamera(Actor* camera) { mainCamera_ = camera; }
    
    // Tick мира (обновление всех Actors)
    void Tick(float deltaTime);
    
    // Callback при создании/уничтожении Actor
    using ActorCreatedCallback = std::function<void(Actor*)>;
    using ActorDestroyedCallback = std::function<void(Actor*)>;
    
    void OnActorCreated(ActorCreatedCallback callback) { onActorCreated_ = callback; }
    void OnActorDestroyed(ActorDestroyedCallback callback) { onActorDestroyed_ = callback; }
    
private:
    std::vector<Actor*> actors_;
    std::unordered_map<uint64_t, Actor*> actorsById_;
    Actor* mainCamera_ = nullptr;
    
    uint64_t nextActorId_ = 1;
    
    ActorCreatedCallback onActorCreated_;
    ActorDestroyedCallback onActorDestroyed_;
};

// ============================================================================
// WorldManager - управляет несколькими мирами (уровнями)
// ============================================================================
class WorldManager {
public:
    static WorldManager& Get() {
        static WorldManager instance;
        return instance;
    }
    
    // Создать новый мир
    World* CreateWorld(const std::string& name = "World");
    
    // Удалить мир
    void DestroyWorld(World* world);
    
    // Получить текущий мир
    World* GetCurrentWorld() const { return currentWorld_; }
    
    // Установить текущий мир
    void SetCurrentWorld(World* world) { currentWorld_ = world; }
    
    // Загрузить мир из файла
    World* LoadWorld(const std::string& filename);
    
    // Сохранить текущий мир
    bool SaveCurrentWorld(const std::string& filename) const;
    
    // Получить все миры
    const std::vector<World*>& GetWorlds() const { return worlds_; }
    
    // Найти мир по имени
    World* FindWorldByName(const std::string& name) const;
    
    // Очистить все миры
    void ClearAllWorlds();
    
    // Tick текущего мира
    void Tick(float deltaTime);
    
private:
    WorldManager() = default;
    ~WorldManager();
    
    std::vector<World*> worlds_;
    World* currentWorld_ = nullptr;
};

} // namespace eoa
