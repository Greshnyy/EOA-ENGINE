#pragma once

#include "core/Systems.h"
#include "events/EventSystem.h"
#include "input/InputSystem.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace eoa {

// ============================================================================
// КОНФИГУРАЦИЯ ДВИЖКА
// ============================================================================

struct EngineConfig {
    std::string title = "EOA Engine";
    int width = 1920;
    int height = 1080;
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    bool fixedTimestep = true;
    LogLevel logLevel = LogLevel::Info;
    std::string contentPath = "./Content";
    std::string projectFile;
};

// ============================================================================
// МЕНЕДЖЕР СИСТЕМ
// ============================================================================

class SystemManager {
public:
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        systems_.push_back(system);
        
        // Сортировка по приоритету
        std::sort(systems_.begin(), systems_.end(),
            [](const auto& a, const auto& b) {
                return a->GetPriority() < b->GetPriority();
            });
        
        if (initialized_) {
            system->Initialize();
        }
        
        return system.get();
    }
    
    template<typename T>
    T* GetSystem() {
        for (auto& sys : systems_) {
            if (auto typed = std::dynamic_pointer_cast<T>(sys)) {
                return typed.get();
            }
        }
        return nullptr;
    }
    
    void Initialize() {
        for (auto& sys : systems_) {
            sys->Initialize();
        }
        initialized_ = true;
    }
    
    void Shutdown() {
        // Обратный порядок для shutdown
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            (*it)->Shutdown();
        }
        systems_.clear();
        initialized_ = false;
    }
    
    void Update(Seconds dt) {
        for (auto& sys : systems_) {
            if (sys->IsEnabled()) {
                sys->Update(dt);
            }
        }
    }
    
    void FixedUpdate(Seconds dt) {
        for (auto& sys : systems_) {
            if (sys->IsEnabled()) {
                sys->FixedUpdate(dt);
            }
        }
    }
    
    void Render() {
        for (auto& sys : systems_) {
            if (sys->IsEnabled()) {
                sys->Render();
            }
        }
    }

private:
    std::vector<std::shared_ptr<ISystem>> systems_;
    bool initialized_ = false;
};

// ============================================================================
// GAME INSTANCE (сохраняется между уровнями)
// ============================================================================

class GameInstance : public IEventListener {
public:
    virtual ~GameInstance() = default;
    
    void OnEvent(Event& event) override {
        // Обработка событий на уровне приложения
    }
    
    // Переопредели для инициализации
    virtual void Init() {}
    virtual void Shutdown() {}
    
    // Вызывается при загрузке нового уровня
    virtual void OnLevelLoaded(const std::string& levelName) {}
    
    // Вызывается при выгрузке уровня
    virtual void OnLevelUnloaded(const std::string& levelName) {}
    
    // Глобальное состояние игры
    template<typename T>
    void SetState(const std::string& key, T value) {
        state_[key] = std::move(value);
    }
    
    template<typename T>
    T* GetState(const std::string& key) {
        auto it = state_.find(key);
        if (it != state_.end()) {
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, std::any> state_;
};

// ============================================================================
// ОСНОВНОЙ КЛАСС ДВИЖКА
// ============================================================================

class Engine {
public:
    static Engine& GetInstance() {
        static Engine instance;
        return instance;
    }
    
    // Инициализация
    bool Initialize(const EngineConfig& config) {
        config_ = config;
        
        // Настройка логгера
        Logger::GetInstance().SetLevel(config.logLevel);
        
        EOA_LOG_INFO("Инициализация EOA Engine...");
        EOA_LOG_INFO("Название: " + config.title);
        EOA_LOG_INFO("Разрешение: " + std::to_string(config.width) + "x" + 
                     std::to_string(config.height));
        
        // Добавление базовых систем
        inputSystem_ = systemManager_.AddSystem<InputSystem>();
        
        // Инициализация менеджера систем
        systemManager_.Initialize();
        
        EOA_LOG_INFO("Движок успешно инициализирован");
        return true;
    }
    
    // Главный цикл
    void Run() {
        running_ = true;
        
        EOA_LOG_INFO("Запуск главного цикла...");
        
        while (running_) {
            // Обновление времени
            timeSystem_.Tick();
            
            // Начало кадра для ввода
            inputSystem_->BeginFrame();
            
            // Обработка событий окна
            ProcessEvents();
            
            // Обработка отложенных событий
            eventDispatcher_.DispatchPending();
            
            // Обновление задач
            TaskManager::GetInstance().ProcessTasks();
            
            // Fixed update (физика и т.д.)
            if (config_.fixedTimestep && timeSystem_.ShouldFixedUpdate()) {
                timeSystem_.DoFixedUpdate();
                systemManager_.FixedUpdate(config_.fixedDeltaTime);
            }
            
            // Update
            systemManager_.Update(timeSystem_.GetTime().deltaTime);
            
            // Render
            systemManager_.Render();
            
            // Ограничение FPS
            if (config_.targetFPS > 0) {
                double frameTime = 1.0 / config_.targetFPS;
                double elapsed = timeSystem_.GetTime().realDeltaTime_;
                if (elapsed < frameTime) {
                    TimeSystem::Sleep((frameTime - elapsed) * 1000.0);
                }
            }
        }
        
        Shutdown();
    }
    
    // Остановка движка
    void Quit() {
        running_ = false;
    }
    
    // Shutdown
    void Shutdown() {
        EOA_LOG_INFO("Остановка движка...");
        
        systemManager_.Shutdown();
        
        EOA_LOG_INFO("Движок остановлен");
    }
    
    // Доступ к системам
    EventDispatcher& GetEventDispatcher() { return eventDispatcher_; }
    SystemManager& GetSystemManager() { return systemManager_; }
    InputSystem* GetInputSystem() { return inputSystem_; }
    const Time& GetTime() const { return timeSystem_.GetTime(); }
    const EngineConfig& GetConfig() const { return config_; }
    
    // Game Instance
    template<typename T>
    void SetGameInstance(std::unique_ptr<T> instance) {
        gameInstance_ = std::move(instance);
        if (gameInstance_) {
            gameInstance_->Init();
            eventDispatcher_.AddListener(gameInstance_.get());
        }
    }
    
    template<typename T>
    T* GetGameInstance() {
        return dynamic_cast<T*>(gameInstance_.get());
    }
    
    // Статические методы для удобного доступа
    static Engine& Get() { return GetInstance(); }
    static EventDispatcher& Events() { return Get().GetEventDispatcher(); }
    static InputSystem* Input() { return Get().GetInputSystem(); }
    static const Time& Time() { return Get().GetTime(); }

private:
    Engine() = default;
    ~Engine() = default;
    
    // Запрет копирования
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    void ProcessEvents() {
        // Здесь будет обработка событий платформы (GLFW/SDL)
        // Пока заглушка
    }
    
    EngineConfig config_;
    TimeSystem timeSystem_;
    SystemManager systemManager_;
    EventDispatcher eventDispatcher_;
    InputSystem* inputSystem_ = nullptr;
    std::unique_ptr<GameInstance> gameInstance_;
    bool running_ = false;
    bool initialized_ = false;
};

// Макрос для глобального доступа
#define gEngine eoa::Engine::Get()
#define gEvents eoa::Engine::Events()
#define gInput eoa::Engine::Input()
#define gTime eoa::Engine::Time()

} // namespace eoa
