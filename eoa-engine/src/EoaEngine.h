#pragma once

#include "core/Engine.h"
#include "core/Systems.h"
#include "events/EventSystem.h"
#include "input/InputSystem.h"
#include "world/World.h"
#include "world/Level.h"
#include "resources/ResourceManager.h"

#define EOA_ENGINE_VERSION_MAJOR 0
#define EOA_ENGINE_VERSION_MINOR 1
#define EOA_ENGINE_VERSION_PATCH 0

namespace eoa {

inline const char* GetVersion() { return "0.1.0"; }

inline bool Initialize(const EngineConfig& config = EngineConfig()) {
    return Engine::GetInstance().Initialize(config);
}

inline void Run() { Engine::GetInstance().Run(); }
inline void Quit() { Engine::GetInstance().Quit(); }

inline EventDispatcher& GetEventDispatcher() { return Engine::Get().GetEventDispatcher(); }
inline InputSystem* GetInputSystem() { return Engine::Get().GetInputSystem(); }

inline LevelManager& GetLevelManager() {
    static LevelManager instance;
    return instance;
}

inline ResourceManager& GetResourceManager() {
    static ResourceManager instance;
    return instance;
}

} // namespace eoa

#define EOA_LOG(msg) EOA_LOG_INFO(msg)
#define GET_ENGINE() eoa::Engine::Get()
#define GET_INPUT() eoa::Engine::Input()
#define GET_TIME() eoa::Engine::Get().GetTime()
#define GET_EVENTS() eoa::Engine::Events()
#define GET_LEVEL_MANAGER() eoa::GetLevelManager()
#define GET_RESOURCE_MANAGER() eoa::GetResourceManager()
