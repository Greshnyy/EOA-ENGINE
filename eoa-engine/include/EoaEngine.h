#pragma once

// ============================================
// EOA ENGINE - Главный заголовочный файл
// ============================================
// Включает все основные системы движка
// Использование: #include "EoaEngine.h"
// ============================================

#include "Core/Platform.h"
#include "Core/Types.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Core/Input.h"
#include "Core/Events.h"
#include "Core/Engine.h"
#include "Core/World.h"
#include "Core/ResourceManager.h"

#include "Math/Vector.h"
#include "Math/Matrix.h"

#include "Render/Renderer.h"
#include "Resources/Resource.h"

// ============================================
// ГЛОБАЛЬНЫЕ МАКРОСЫ (в стиле UE)
// ============================================

#define gEngine EOA::Engine::GetInstance()
#define gWorld EOA::World::GetInstance()
#define gResources EOA::ResourceManager::GetInstance()
#define gInput EOA::InputSystem::GetInstance()
#define gTime EOA::TimeSystem::GetInstance()
#define gLogger EOA::Logger::GetInstance()

// Логирование
#define EOA_LOG_TRACE(msg) EOA_LOG_TRACE(msg)
#define EOA_LOG_DEBUG(msg) EOA_LOG_DEBUG(msg)
#define EOA_LOG_INFO(msg)  EOA_LOG_INFO(msg)
#define EOA_LOG_WARN(msg)  EOA_LOG_WARN(msg)
#define EOA_LOG_ERROR(msg) EOA_LOG_ERROR(msg)
#define EOA_LOG_CRITICAL(msg) EOA_LOG_CRITICAL(msg)

// Время
#define DeltaTime() EOA::TimeSystem::GetInstance()->DeltaTime()
#define TotalTime() EOA::TimeSystem::GetInstance()->TotalTime()
#define GetFPS() EOA::TimeSystem::GetInstance()->FPS()

// Ввод
#define IsKeyPressed(key) EOA::InputSystem::GetInstance()->IsKeyPressed(key)
#define IsKeyJustPressed(key) EOA::InputSystem::GetInstance()->IsKeyJustPressed(key)
#define GetMouseX() EOA::InputSystem::GetInstance()->GetMouseX()
#define GetMouseY() EOA::InputSystem::GetInstance()->GetMouseY()

// Ресурсы
#define LoadResource<T>(path) EOA::ResourceManager::GetInstance()->LoadResource<T>(path)
#define GetResource<T>(path) EOA::ResourceManager::GetInstance()->GetResource<T>(path)

// Спавн акторов
#define SpawnActor<T>() EOA::World::GetInstance()->SpawnActor<T>()

// ============================================
// ВЕРСИЯ ДВИЖКА
// ============================================

namespace EOA {

const char* GetVersionString() {
    return EOA_VERSION_STRING;
}

int GetVersionMajor() { return EOA_VERSION_MAJOR; }
int GetVersionMinor() { return EOA_VERSION_MINOR; }
int GetVersionPatch() { return EOA_VERSION_PATCH; }

} // namespace EOA

// ============================================
// ПРИМЕР ИСПОЛЬЗОВАНИЯ
// ============================================
/*
#include "EoaEngine.h"

class MyGame : public EOA::Application {
public:
    static void Configure(EOA::EngineConfig& config) {
        config.Title = "My Game";
        config.Width = 1920;
        config.Height = 1080;
        config.EditorMode = false;
    }
};

EOA_IMPLEMENT_APPLICATION(MyGame);
*/
