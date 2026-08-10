#include "core/world.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>

namespace eoa {

using json = nlohmann::json;

World::World() : nextActorId_(1), mainCamera_(nullptr) {}
World::~World() { Clear(); }

Actor* World::CreateActor(const std::string& name) {
    auto* actor = new Actor();
    actor->SetName(name);
    actor->SetId(nextActorId_++);
    actor->SetWorld(this);
    actors_.push_back(actor);
    actorsById_[actor->GetId()] = actor;
    if (onActorCreated_) onActorCreated_(actor);
    return actor;
}

void World::DestroyActor(Actor* actor) {
    if (!actor) return;
    const auto it = std::find(actors_.begin(), actors_.end(), actor);
    if (it == actors_.end()) return;
    actors_.erase(it);
    actorsById_.erase(actor->GetId());
    if (mainCamera_ == actor) mainCamera_ = nullptr;
    if (onActorDestroyed_) onActorDestroyed_(actor);
    delete actor;
}

Actor* World::FindActorByName(const std::string& name) const {
    for (auto* actor : actors_) {
        if (actor && actor->GetName() == name) return actor;
    }
    return nullptr;
}

Actor* World::FindActorById(uint64_t id) const {
    const auto it = actorsById_.find(id);
    return it != actorsById_.end() ? it->second : nullptr;
}

void World::Clear() {
    for (auto it = actors_.rbegin(); it != actors_.rend(); ++it) {
        if (onActorDestroyed_) onActorDestroyed_(*it);
        delete *it;
    }
    actors_.clear();
    actorsById_.clear();
    mainCamera_ = nullptr;
    nextActorId_ = 1;
}

std::string World::Serialize() const {
    json root;
    root["__type__"] = "World";
    root["__version__"] = 1;

    json actorsJson = json::array();
    for (auto* actor : actors_) {
        if (!actor) continue;
        json actorJson;
        actorJson["name"] = actor->GetName();
        actorJson["id"] = actor->GetId();
        actorJson["active"] = actor->IsActive();

        json componentsJson = json::array();
        for (const auto& comp : actor->GetComponents()) {
            if (!comp) continue;
            json compJson;
            compJson["__type__"] = comp->ClassName();
            auto cls = ReflectionSystem::Get().GetClass(comp->ClassName());
            if (cls) {
                json propsJson;
                for (const auto& [propName, prop] : cls->GetProperties()) {
                    (void)prop;
                    propsJson[propName] = "serialized_value";
                }
                compJson["properties"] = propsJson;
            }
            componentsJson.push_back(compJson);
        }
        actorJson["components"] = componentsJson;

        json childrenJson = json::array();
        for (auto* child : actor->GetChildren()) {
            if (child) childrenJson.push_back(child->GetId());
        }
        actorJson["children"] = childrenJson;
        if (actor->GetParent()) actorJson["parent"] = actor->GetParent()->GetId();
        actorsJson.push_back(actorJson);
    }

    root["actors"] = actorsJson;
    if (mainCamera_) root["mainCamera"] = mainCamera_->GetId();
    return root.dump(4);
}

bool World::Deserialize(const std::string& data) {
    try {
        const json root = json::parse(data);
        if (!root.is_object() || root.value("__type__", "") != "World") {
            LOG_ERROR("Invalid world file: expected World root object");
            return false;
        }

        Clear();
        std::unordered_map<uint64_t, Actor*> loadedActors;
        uint64_t maxActorId = 0;

        if (root.contains("actors") && root["actors"].is_array()) {
            for (const auto& actorJson : root["actors"]) {
                const uint64_t id = actorJson.value("id", uint64_t{0});
                if (id == 0 || loadedActors.contains(id)) {
                    LOG_ERROR("World contains a missing or duplicate actor ID");
                    Clear();
                    return false;
                }

                auto* actor = CreateActor(actorJson.value("name", "Actor"));
                actorsById_.erase(actor->GetId());
                actor->SetId(id);
                actorsById_[id] = actor;
                actor->SetActive(actorJson.value("active", true));
                loadedActors[id] = actor;
                maxActorId = std::max(maxActorId, id);
            }

            nextActorId_ = maxActorId == std::numeric_limits<uint64_t>::max() ? 1 : maxActorId + 1;

            for (const auto& actorJson : root["actors"]) {
                const uint64_t id = actorJson.value("id", uint64_t{0});
                const auto it = loadedActors.find(id);
                if (it == loadedActors.end()) continue;
                auto* actor = it->second;

                if (actorJson.contains("parent")) {
                    const uint64_t parentId = actorJson["parent"].get<uint64_t>();
                    const auto parentIt = loadedActors.find(parentId);
                    if (parentIt != loadedActors.end() && parentIt->second != actor) {
                        actor->SetParent(parentIt->second);
                    }
                }
            }
        }

        if (root.contains("mainCamera")) {
            const uint64_t cameraId = root["mainCamera"].get<uint64_t>();
            const auto it = loadedActors.find(cameraId);
            if (it != loadedActors.end()) mainCamera_ = it->second;
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
    if (auto obj = serializer.LoadFromFile(filename, "World")) {
        delete obj;
        return true;
    }

    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    return Deserialize(buffer.str());
}

void World::Tick(float deltaTime) {
    for (auto* actor : actors_) {
        if (actor && actor->IsActive()) actor->Tick(deltaTime);
    }
}

World* WorldManager::CreateWorld(const std::string& name) {
    auto* world = new World();
    world->SetName(name);
    worlds_.push_back(world);
    if (!currentWorld_) currentWorld_ = world;
    return world;
}

void WorldManager::DestroyWorld(World* world) {
    if (!world) return;
    const auto it = std::find(worlds_.begin(), worlds_.end(), world);
    if (it == worlds_.end()) return;
    worlds_.erase(it);
    if (currentWorld_ == world) currentWorld_ = worlds_.empty() ? nullptr : worlds_.front();
    delete world;
}

World* WorldManager::LoadWorld(const std::string& filename) {
    auto* world = new World();
    if (!world->LoadFromFile(filename)) {
        delete world;
        return nullptr;
    }
    worlds_.push_back(world);
    currentWorld_ = world;
    return world;
}

bool WorldManager::SaveCurrentWorld(const std::string& filename) const {
    return currentWorld_ && currentWorld_->SaveToFile(filename);
}

World* WorldManager::FindWorldByName(const std::string& name) const {
    for (auto* world : worlds_) {
        if (world && world->GetName() == name) return world;
    }
    return nullptr;
}

void WorldManager::ClearAllWorlds() {
    for (auto* world : worlds_) delete world;
    worlds_.clear();
    currentWorld_ = nullptr;
}

void WorldManager::Tick(float deltaTime) {
    if (currentWorld_) currentWorld_->Tick(deltaTime);
}

WorldManager::~WorldManager() { ClearAllWorlds(); }

EOA_CLASS_IMPL(World, Object)
EOA_END_CLASS_IMPL()
EOA_REGISTER_CLASS(World)

} // namespace eoa
