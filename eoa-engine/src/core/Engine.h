#pragma once

#include "core/Systems.h"
#include "events/EventSystem.h"
#include "input/InputSystem.h"
#include "platform/window.h"
#include "renderer/renderer.h"
#include "rhi/vk_device.h"
#include "rhi/vk_instance.h"

#include <algorithm>
#include <any>
#include <glm/glm.hpp>

#include <algorithm>
#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eoa {

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
    bool enableVulkanValidation = true;
};

class SystemManager {
public:
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        systems_.push_back(system);
        std::sort(systems_.begin(), systems_.end(), [](const auto& a, const auto& b) {
            return a->GetPriority() < b->GetPriority();
        });
        if (initialized_) system->Initialize();

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
            if (auto typed = std::dynamic_pointer_cast<T>(sys)) return typed.get();
        }
        return nullptr;
    }

    void Initialize() {
        for (auto& sys : systems_) sys->Initialize();
        initialized_ = true;
    }

    void Shutdown() {
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) (*it)->Shutdown();
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) {
            (*it)->Shutdown();
        }
        systems_.clear();
        initialized_ = false;
    }

    void Update(Seconds dt) {
        for (auto& sys : systems_) if (sys->IsEnabled()) sys->Update(dt);
    }

    void FixedUpdate(Seconds dt) {
        for (auto& sys : systems_) if (sys->IsEnabled()) sys->FixedUpdate(dt);
    }

    void Render() {
        for (auto& sys : systems_) if (sys->IsEnabled()) sys->Render();
    }

private:
    std::vector<std::shared_ptr<ISystem>> systems_;
    bool initialized_ = false;
};

class GameInstance : public IEventListener {
public:
    virtual ~GameInstance() = default;
    void OnEvent(Event&) override {}
    virtual void Init() {}
    virtual void Shutdown() {}

    void OnEvent(Event&) override {
        // Обработка событий на уровне приложения
    }

    virtual void Init() {}
    virtual void Shutdown() {}

    virtual void OnLevelLoaded(const std::string& levelName) { (void)levelName; }
    virtual void OnLevelUnloaded(const std::string& levelName) { (void)levelName; }

    template<typename T>
    void SetState(const std::string& key, T value) { state_[key] = std::move(value); }
    void SetState(const std::string& key, T value) {
        state_[key] = std::move(value);
    }

    template<typename T>
    T* GetState(const std::string& key) {
        auto it = state_.find(key);
        return it != state_.end() ? std::any_cast<T>(&it->second) : nullptr;
    }

private:
    std::unordered_map<std::string, std::any> state_;
};

class Engine {
public:
    static Engine& GetInstance() {
        static Engine instance;
        return instance;
    }

    bool Initialize(const EngineConfig& config) {
        if (initialized_) {
            EOA_LOG_WARN("Engine is already initialized");
            return true;
        }

        config_ = config;
        config_.width = std::max(1, config_.width);
        config_.height = std::max(1, config_.height);
        config_.fixedDeltaTime = std::max(0.0001, config_.fixedDeltaTime);

        Logger::GetInstance().SetLevel(config_.logLevel);
        timeSystem_.SetFixedDeltaTime(config_.fixedDeltaTime);

        EOA_LOG_INFO("Initializing EOA Engine...");
        EOA_LOG_INFO("Window: " + std::to_string(config_.width) + "x" +
                     std::to_string(config_.height));

        window_ = std::make_unique<Window>(config_.width, config_.height, config_.title);
        instance_ = std::make_unique<VulkanInstance>(config_.enableVulkanValidation);
        surface_ = window_->CreateSurface(instance_->Handle());
        device_ = std::make_unique<VulkanDevice>(instance_->Handle(), surface_);
        renderer_ = std::make_unique<Renderer>(
            device_->Physical(), device_->Logical(), surface_,
            device_->GraphicsQueueFamily(), device_->GraphicsQueue(),
            static_cast<uint32_t>(config_.width), static_cast<uint32_t>(config_.height));

        inputSystem_ = systemManager_.AddSystem<InputSystem>();
        eventDispatcher_.AddListener(inputSystem_);
        systemManager_.Initialize();

        initialized_ = true;
        EOA_LOG_INFO("EOA Engine initialized successfully");

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
        if (!initialized_) {
            EOA_LOG_ERROR("Engine::Run called before Initialize");
            return;
        }

        running_ = true;
        EOA_LOG_INFO("Starting main loop...");

        while (running_ && window_ && !window_->ShouldClose()) {
            timeSystem_.Tick();
            ProcessEvents();

            if (inputSystem_) inputSystem_->BeginFrame();

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

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            window_->FramebufferSize(framebufferWidth, framebufferHeight);
            if (framebufferWidth > 0 && framebufferHeight > 0 && renderer_) {
                renderer_->GetWorld().Tick(static_cast<float>(timeSystem_.GetTime().deltaTime));
                renderer_->DrawFrame(
                    static_cast<uint32_t>(framebufferWidth),
                    static_cast<uint32_t>(framebufferHeight),
                    glm::mat4(1.0f));
            systemManager_.Render();

            if (config_.targetFPS > 0) {
                const double frameTime = 1.0 / static_cast<double>(config_.targetFPS);
                const double elapsed = timeSystem_.GetTime().realDeltaTime_;
                if (elapsed < frameTime) {
                    TimeSystem::Sleep((frameTime - elapsed) * 1000.0);
                }
            }

            systemManager_.Render();
            LimitFrameRate();
        }

        Shutdown();
    }

    void Quit() {
        running_ = false;
        if (window_ && window_->Handle()) glfwSetWindowShouldClose(window_->Handle(), GLFW_TRUE);
    }

    void Shutdown() {
        if (!initialized_) return;

        EOA_LOG_INFO("Shutting down EOA Engine...");
        running_ = false;

        if (device_) vkDeviceWaitIdle(device_->Logical());
        if (gameInstance_) gameInstance_->Shutdown();
        systemManager_.Shutdown();
        TaskManager::GetInstance().ClearTasks();

        // Reverse Vulkan ownership order: renderer -> device -> surface -> instance -> window.
        renderer_.reset();
        device_.reset();
        if (surface_ != VK_NULL_HANDLE && instance_) {
            vkDestroySurfaceKHR(instance_->Handle(), surface_, nullptr);
        }
        surface_ = VK_NULL_HANDLE;
        instance_.reset();
        window_.reset();

        inputSystem_ = nullptr;
        initialized_ = false;
        EOA_LOG_INFO("EOA Engine stopped");
        EOA_LOG_INFO("Остановка движка...");
        systemManager_.Shutdown();
        EOA_LOG_INFO("Движок остановлен");
    }

    EventDispatcher& GetEventDispatcher() { return eventDispatcher_; }
    SystemManager& GetSystemManager() { return systemManager_; }
    InputSystem* GetInputSystem() { return inputSystem_; }
    Renderer* GetRenderer() { return renderer_.get(); }
    Window* GetWindow() { return window_.get(); }
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
    T* GetGameInstance() { return dynamic_cast<T*>(gameInstance_.get()); }
    T* GetGameInstance() {
        return dynamic_cast<T*>(gameInstance_.get());
    }

    static Engine& Get() { return GetInstance(); }
    static EventDispatcher& Events() { return Get().GetEventDispatcher(); }
    static InputSystem* Input() { return Get().GetInputSystem(); }
    static const Time& Time() { return Get().GetTime(); }

private:
    Engine() = default;
    ~Engine() { Shutdown(); }
    ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void ProcessEvents() {
        if (!window_) return;
        window_->PollEvents();
    }

    void LimitFrameRate() {
        if (config_.targetFPS <= 0) return;
        const double targetFrame = 1.0 / static_cast<double>(config_.targetFPS);
        const double elapsed = timeSystem_.GetTime().GetRealDeltaTime();
        if (elapsed < targetFrame) {
            TimeSystem::Sleep((targetFrame - elapsed) * 1000.0);
        }
    }

    EngineConfig config_;
    TimeSystem timeSystem_;
    SystemManager systemManager_;
    EventDispatcher eventDispatcher_;

    std::unique_ptr<Window> window_;
    std::unique_ptr<VulkanInstance> instance_;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    std::unique_ptr<VulkanDevice> device_;
    std::unique_ptr<Renderer> renderer_;

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
