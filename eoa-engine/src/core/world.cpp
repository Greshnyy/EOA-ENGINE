#include "core/world.h"
#include "third_party/json.hpp"
#include <algorithm>

namespace eoa {

using json = nlohmann::json;

// ============================================================================
// World implementation
// ============================================================================

World::World() 
    : nextActorId_(1)
    , mainCamera_(nullptr)
{
}

World::~World() {
    Clear();
}

Actor* World::CreateActor(const std::string& name) {
    auto actor = new Actor();
    actor->SetName(name);
    actor->SetId(nextActorId_++);
    actor->SetWorld(this);
    
    actors_.push_back(actor);
    actorsById_[actor->GetId()] = actor;
    
    if (onActorCreated_) {
        onActorCreated_(actor);
    }
    
    return actor;
}

void World::DestroyActor(Actor* actor) {
    if (!actor) return;
    
    // Удалить из массива
    auto it = std::find(actors_.begin(), actors_.end(), actor);
    if (it != actors_.end()) {
        actors_.erase(it);
    }
    
    // Удалить из map по ID
    actorsById_.erase(actor->GetId());
    
    // Если это была main camera, сбросить
    if (mainCamera_ == actor) {
        mainCamera_ = nullptr;
    }
    
    if (onActorDestroyed_) {
        onActorDestroyed_(actor);
    }
    
    delete actor;
}

Actor* World::FindActorByName(const std::string& name) const {
    for (auto* actor : actors_) {
        if (actor->GetName() == name) {
            return actor;
        }
    }
    return nullptr;
}

Actor* World::FindActorById(uint64_t id) const {
    auto it = actorsById_.find(id);
    return it != actorsById_.end() ? it->second : nullptr;
}

void World::Clear() {
    // Удалить все Actors в обратном порядке (для корректного уничтожения)
    for (auto it = actors_.rbegin(); it != actors_.rend(); ++it) {
        if (onActorDestroyed_) {
            onActorDestroyed_(*it);
        }
        delete *it;
    }
    
    actors_.clear();
    actorsById_.clear();
    mainCamera_ = nullptr;
}

std::string World::Serialize() const {
    json root;
    root["__type__"] = "World";
    root["__version__"] = 1;
    
    // Сериализация всех Actors
    json actorsJson = json::array();
    for (auto* actor : actors_) {
        if (!actor) continue;
        
        json actorJson;
        actorJson["name"] = actor->GetName();
        actorJson["id"] = actor->GetId();
        actorJson["active"] = actor->IsActive();
        
        // Сериализация компонентов
        json componentsJson = json::array();
        const auto& components = actor->GetComponents();
        for (const auto& comp : components) {
            if (!comp) continue;
            
            json compJson;
            compJson["__type__"] = comp->ClassName();
            
            // Сериализация свойств компонента через рефлексию
            auto cls = ReflectionSystem::Get().GetClass(comp->ClassName());
            if (cls) {
                json propsJson;
                for (const auto& [propName, prop] : cls->GetProperties()) {
                    json propValue;
                    // Здесь нужна функция для сериализации свойства
                    // Пока упрощённо
                    propsJson[propName] = "serialized_value";
                }
                compJson["properties"] = propsJson;
            }
            
            componentsJson.push_back(compJson);
        }
        actorJson["components"] = componentsJson;
        
        // Сериализация детей
        json childrenJson = json::array();
        const auto& children = actor->GetChildren();
        for (auto* child : children) {
            childrenJson.push_back(child->GetId());
        }
        actorJson["children"] = childrenJson;
        
        // Родитель
        if (actor->GetParent()) {
            actorJson["parent"] = actor->GetParent()->GetId();
        }
        
        actorsJson.push_back(actorJson);
    }
    
    root["actors"] = actorsJson;
    
    // Main camera
    if (mainCamera_) {
        root["mainCamera"] = mainCamera_->GetId();
    }
    
    return root.dump(4);
}

bool World::Deserialize(const std::string& data) {
    try {
        json root = json::parse(data);
        
        // Очистить текущий мир
        Clear();
        
        // Десериализация Actors
        if (root.contains("actors")) {
            std::unordered_map<uint64_t, Actor*> actorsById;
            
            // Первый проход: создание всех Actors
            for (const auto& actorJson : root["actors"]) {
                auto* actor = CreateActor(actorJson.value("name", "Actor"));
                actor->SetId(actorJson.value("id", 0));
                actor->SetActive(actorJson.value("active", true));
                actorsById[actor->GetId()] = actor;
            }
            
            // Второй проход: установка иерархии и компонентов
            for (const auto& actorJson : root["actors"]) {
                uint64_t id = actorJson.value("id", 0);
                auto it = actorsById.find(id);
                if (it == actorsById.end()) continue;
                
                auto* actor = it->second;
                
                // Родитель
                if (actorJson.contains("parent")) {
                    uint64_t parentId = actorJson["parent"];
                    auto pit = actorsById.find(parentId);
                    if (pit != actorsById.end()) {
                        actor->SetParent(pit->second);
                    }
                }
                
                // Компоненты
                if (actorJson.contains("components")) {
                    for (const auto& compJson : actorJson["components"]) {
                        std::string compType = compJson.value("__type__", "");
                        if (!compType.empty()) {
                            // Создание компонента через рефлексию
                            // TODO: реализовать добавление компонентов
                        }
                    }
                }
            }
            
            // Main camera
            if (root.contains("mainCamera")) {
                uint64_t camId = root["mainCamera"];
                auto it = actorsById.find(camId);
                if (it != actorsById.end()) {
                    mainCamera_ = it->second;
                }
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("World deserialization error: {}", e.what());
        return false;
    }
}

bool World::SaveToFile(const std::string& filename) const {
    JsonSerializer serializer;
    return serializer.SaveToFile(const_cast<World*>(this), filename);
}

bool World::LoadFromFile(const std::string& filename) {
    JsonSerializer serializer;
    auto obj = serializer.LoadFromFile(filename, "World");
    if (obj) {
        // Копирование данных из загруженного объекта в текущий
        delete obj;
        return true;
    }
    
    // Fallback: прямая загрузка JSON
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return Deserialize(buffer.str());
}

void World::Tick(float deltaTime) {
    // Обновление всех активных Actors
    for (auto* actor : actors_) {
        if (actor && actor->IsActive()) {
            actor->Tick(deltaTime);
        }
    }
}

// ============================================================================
// WorldManager implementation
// ============================================================================

World* WorldManager::CreateWorld(const std::string& name) {
    auto* world = new World();
    world->SetName(name);
    worlds_.push_back(world);
    
    if (!currentWorld_) {
        currentWorld_ = world;
    }
    
    return world;
}

void WorldManager::DestroyWorld(World* world) {
    if (!world) return;
    
    auto it = std::find(worlds_.begin(), worlds_.end(), world);
    if (it != worlds_.end()) {
        worlds_.erase(it);
    }
    
    if (currentWorld_ == world) {
        currentWorld_ = worlds_.empty() ? nullptr : worlds_[0];
    }
    
    delete world;
}

World* WorldManager::LoadWorld(const std::string& filename) {
    auto* world = new World();
    if (world->LoadFromFile(filename)) {
        worlds_.push_back(world);
        currentWorld_ = world;
        return world;
    }
    
    delete world;
    return nullptr;
}

bool WorldManager::SaveCurrentWorld(const std::string& filename) const {
    if (!currentWorld_) {
        return false;
    }
    return currentWorld_->SaveToFile(filename);
}

World* WorldManager::FindWorldByName(const std::string& name) const {
    for (auto* world : worlds_) {
        if (world->GetName() == name) {
            return world;
        }
    }
    return nullptr;
}

void WorldManager::ClearAllWorlds() {
    for (auto* world : worlds_) {
        delete world;
    }
    worlds_.clear();
    currentWorld_ = nullptr;
}

void WorldManager::Tick(float deltaTime) {
    if (currentWorld_) {
        currentWorld_->Tick(deltaTime);
    }
}

WorldManager::~WorldManager() {
    ClearAllWorlds();
}

// Регистрация класса World
EOA_CLASS_IMPL(World, Object)
EOA_END_CLASS_IMPL()

EOA_REGISTER_CLASS(World)

} // namespace eoa
