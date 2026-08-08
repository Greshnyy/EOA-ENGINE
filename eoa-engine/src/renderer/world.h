#pragma once
#include <memory>
#include <vector>
#include <string>
#include "core/actor.h"

namespace eoa {

// World — контейнер всех Actor-ов на уровне, аналог UWorld из UE.
// Заменяет старый плоский Scene (GameObject + Material).
// Actor-ы владеют своими компонентами (Transform, Mesh, Camera, Light и т.д.)
// через AddComponent<T>().
class World {
public:
    std::vector<std::unique_ptr<Actor>> actors;

    template<typename T = Actor, typename... Args>
    T* SpawnActor(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        actor->SetName(name);
        T* raw = actor.get();
        actors.push_back(std::move(actor));
        return raw;
    }

    void DestroyActor(Actor* actor) {
        actors.erase(
            std::remove_if(actors.begin(), actors.end(),
                [actor](const auto& a) { return a.get() == actor; }),
            actors.end());
    }

    void Clear() {
        actors.clear();
    }

    void BeginPlay() {
        for (auto& actor : actors) {
            actor->BeginPlay();
        }
    }

    void Tick(float deltaTime) {
        for (auto& actor : actors) {
            actor->Tick(deltaTime);
        }
    }
};

} // namespace eoa
