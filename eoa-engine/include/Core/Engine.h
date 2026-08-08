#pragma once

#include "Core/Platform.h"
#include "Core/Types.h"
#include "Core/Events.h"
#include "Core/Input.h"
#include "Core/Time.h"
#include "Core/Logger.h"
#include "Render/Renderer.h"
#include "Resources/Resource.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace EOA {

// ============================================
// КОНФИГУРАЦИЯ ДВИЖКА
// ============================================

struct EngineConfig {
    std::string Title = "EOA Engine";
    int Width = 1920;
    int Height = 1080;
    bool Fullscreen = false;
    bool VSync = true;
    float ClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    std::string ContentPath = "assets/";
    bool EditorMode = false;
};

// ============================================
// БАЗОВЫЙ КЛАСС СИСТЕМЫ
// ============================================

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void Initialize() {}
    virtual void Update(float deltaTime) {}
    virtual void Render() {}
    virtual void Shutdown() {}
    
    // Приоритет обновления (меньше = раньше)
    virtual int GetUpdateOrder() const { return 0; }
};

// ============================================
// ГЛАВНЫЙ КЛАСС ДВИЖКА (в стиле UE)
// ============================================

class EOA_API Engine {
public:
    // Singleton
    static Engine* GetInstance() { return Instance; }
    
    // Инициализация и запуск
    bool Initialize(const EngineConfig& config);
    void Run();
    void Shutdown();
    
    // Конфигурация
    const EngineConfig& GetConfig() const { return Config; }
    
    // Системы
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        system->Initialize();
        T* ptr = system.get();
        Systems.push_back(std::move(system));
        
        // Сортировка по приоритету
        std::sort(Systems.begin(), Systems.end(), 
            [](const auto& a, const auto& b) {
                return a->GetUpdateOrder() < b->GetUpdateOrder();
            });
        
        return ptr;
    }
    
    template<typename T>
    T* GetSystem() {
        for (auto& sys : Systems) {
            if (T* s = dynamic_cast<T*>(sys.get())) {
                return s;
            }
        }
        return nullptr;
    }
    
    // Рендерер
    IRenderer* GetRenderer() { return Renderer.get(); }
    
    // Event Dispatcher
    EventDispatcher& GetEventDispatcher() { return Dispatcher; }
    
    // Состояние
    bool IsRunning() const { return Running; }
    bool IsEditorMode() const { return Config.EditorMode; }
    void Quit() { Running = false; }
    
    // Время
    float GetDeltaTime() const { return TimeSystem::GetInstance()->DeltaTime(); }
    float GetTotalTime() const { return TimeSystem::GetInstance()->TotalTime(); }
    uint32_t GetFPS() const { return TimeSystem::GetInstance()->FPS(); }
    
protected:
    friend class Application;
    
    Engine() = default;
    ~Engine() = default;
    
    void ProcessEvents();
    void Update();
    void Render();
    
private:
    static Engine* Instance;
    
    EngineConfig Config;
    std::unique_ptr<IRenderer> Renderer;
    std::vector<std::unique_ptr<ISystem>> Systems;
    
    EventDispatcher Dispatcher;
    
    bool Running = false;
    bool Initialized = false;
};

// Глобальный доступ к движку (в стиле UE: GEngine)
#define gEngine EOA::Engine::GetInstance()

// ============================================
// МАКРОС ПРИЛОЖЕНИЯ (в стиле UE: IMPLEMENT_MAIN_GAME_TYPE)
// ============================================

class Application {
public:
    static int Run(const EngineConfig& config) {
        Engine* engine = new Engine();
        
        if (!engine->Initialize(config)) {
            EOA_LOG_ERROR("Failed to initialize engine");
            delete engine;
            return -1;
        }
        
        engine->Run();
        engine->Shutdown();
        
        delete engine;
        return 0;
    }
};

#define EOA_IMPLEMENT_APPLICATION(ConfigClass) \
    int main(int argc, char** argv) { \
        EOA::EngineConfig config; \
        ConfigClass::Configure(config); \
        return EOA::Application::Run(config); \
    }

} // namespace EOA
