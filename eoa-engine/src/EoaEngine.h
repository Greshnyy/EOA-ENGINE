#pragma once

// ============================================================================
// EOA ENGINE - Главный заголовочный файл
// ============================================================================
// 
// Архитектура движка в стиле Unreal Engine:
// - Модульная система компонентов
// - Событийно-ориентированная архитектура
// - Рефлексия и сериализация
// - Менеджмент ресурсов с кэшированием
// - Система уровней и миров
// - Гибкая система ввода
// - Логирование и профилирование
// ============================================================================

// Ядро
#include "core/Engine.h"
#include "core/Systems.h"

// События
#include "events/EventSystem.h"

// Ввод
#include "input/InputSystem.h"

// Мир и акторы
#include "world/World.h"
#include "world/Level.h"

// Ресурсы
#include "resources/ResourceManager.h"

// Макросы для удобства
#define EOA_ENGINE_VERSION_MAJOR 0
#define EOA_ENGINE_VERSION_MINOR 1
#define EOA_ENGINE_VERSION_PATCH 0

namespace eoa {

// Версия движка
inline const char* GetVersion() {
    return "0.1.0";
}

// Инициализация движка (удобная функция)
inline bool Initialize(const EngineConfig& config = EngineConfig()) {
    return Engine::GetInstance().Initialize(config);
}

// Запуск главного цикла
inline void Run() {
    Engine::GetInstance().Run();
}

// Остановка движка
inline void Quit() {
    Engine::GetInstance().Quit();
}

// Быстрый доступ к системам
inline EventDispatcher& GetEventDispatcher() {
    return Engine::Get().GetEventDispatcher();
}

inline InputSystem* GetInputSystem() {
    return Engine::Get().GetInputSystem();
}

inline LevelManager& GetLevelManager() {
    static LevelManager instance;
    return instance;
}

inline ResourceManager& GetResourceManager() {
    static ResourceManager instance;
    return instance;
}

} // namespace eoa

// Глобальные макросы (в стиле UE)
#define EOA_LOG(msg) EOA_LOG_INFO(msg)
#define GET_ENGINE() eoa::Engine::Get()
#define GET_INPUT() eoa::Engine::Input()
#define GET_TIME() eoa::Engine::Time()
#define GET_EVENTS() eoa::Engine::Events()
#define GET_LEVEL_MANAGER() eoa::GetLevelManager()
#define GET_RESOURCE_MANAGER() eoa::GetResourceManager()

