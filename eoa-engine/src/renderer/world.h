#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "core/actor.h"

namespace eoa {

// World — контейнер всех Actor-ов на уровне, аналог UWorld из UE.
// Actor-ы владеют своими компонентами (Transform, Mesh, Camera, Light и т.д.)
// через AddComponent<T>().
class World {
public:
    std::vector<std::unique_ptr<Actor>> actors;

    ~World() {
        EndPlay();
    }

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
        if (!actor) {
            return;
        }

        actor->EndPlay();
        actors.erase(
            std::remove_if(actors.begin(), actors.end(),
                [actor](const auto& a) { return a.get() == actor; }),
            actors.end());
    }

    void Clear() {
        EndPlay();
        actors.clear();
    }

    void BeginPlay() {
        for (auto& actor : actors) {
            if (actor) {
                actor->BeginPlay();
            }
        }
    }

    void Tick(float deltaTime) {
        for (auto& actor : actors) {
            if (actor) {
                actor->Tick(deltaTime);
            }
        }
    }

    void EndPlay() {
        for (auto& actor : actors) {
            if (actor) {
                actor->EndPlay();
            }
        }
    }
};

} // namespace eoa
