#pragma once

#include "core/Systems.h"
#include "events/EventSystem.h"
#include "input/InputSystem.h"

#include <algorithm>
#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
    Seconds fixedDeltaTime = 0.02;
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

    void OnEvent(Event&) override {
        // Обработка событий на уровне приложения
    }

    virtual void Init() {}
    virtual void Shutdown() {}

    virtual void OnLevelLoaded(const std::string& levelName) { (void)levelName; }
    virtual void OnLevelUnloaded(const std::string& levelName) { (void)levelName; }

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

    bool Initialize(const EngineConfig& config) {
        config_ = config;

        Logger::GetInstance().SetLevel(config.logLevel);

        EOA_LOG_INFO("Инициализация EOA Engine...");
        EOA_LOG_INFO("Название: " + config.title);
        EOA_LOG_INFO("Разрешение: " + std::to_string(config.width) + "x" +
                     std::to_string(config.height));

        inputSystem_ = systemManager_.AddSystem<InputSystem>();
        systemManager_.Initialize();

        EOA_LOG_INFO("Движок успешно инициализирован");
        return true;
    }

    void Run() {
        running_ = true;

        EOA_LOG_INFO("Запуск главного цикла...");

        while (running_) {
            timeSystem_.Tick();

            if (inputSystem_) {
                inputSystem_->BeginFrame();
            }

            ProcessEvents();
            eventDispatcher_.DispatchPending();
            TaskManager::GetInstance().ProcessTasks(timeSystem_.GetTime().deltaTime);

            if (config_.fixedTimestep) {
                while (timeSystem_.ShouldFixedUpdate()) {
                    timeSystem_.DoFixedUpdate();
                    systemManager_.FixedUpdate(config_.fixedDeltaTime);
                }
            }

            systemManager_.Update(timeSystem_.GetTime().deltaTime);
            systemManager_.Render();

            if (config_.targetFPS > 0) {
                const double frameTime = 1.0 / static_cast<double>(config_.targetFPS);
                const double elapsed = timeSystem_.GetTime().realDeltaTime_;
                if (elapsed < frameTime) {
                    TimeSystem::Sleep((frameTime - elapsed) * 1000.0);
                }
            }
        }

        Shutdown();
    }

    void Quit() {
        running_ = false;
    }

    void Shutdown() {
        EOA_LOG_INFO("Остановка движка...");
        systemManager_.Shutdown();
        EOA_LOG_INFO("Движок остановлен");
    }

    EventDispatcher& GetEventDispatcher() { return eventDispatcher_; }
    SystemManager& GetSystemManager() { return systemManager_; }
    InputSystem* GetInputSystem() { return inputSystem_; }
    const Time& GetTime() const { return timeSystem_.GetTime(); }
    const EngineConfig& GetConfig() const { return config_; }

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

    static Engine& Get() { return GetInstance(); }
    static EventDispatcher& Events() { return Get().GetEventDispatcher(); }
    static InputSystem* Input() { return Get().GetInputSystem(); }
    static const Time& Time() { return Get().GetTime(); }

private:
    Engine() = default;
    ~Engine() = default;

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

#define gEngine eoa::Engine::Get()
#define gEvents eoa::Engine::Events()
#define gInput eoa::Engine::Input()
#define gTime eoa::Engine::Time()

} // namespace eoa
