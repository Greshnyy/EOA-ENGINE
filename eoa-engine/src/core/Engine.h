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
        systems_.push_back(std::move(system));
        std::stable_sort(systems_.begin(), systems_.end(), [](const auto& a, const auto& b) {
            return a->GetPriority() < b->GetPriority();
        });
        if (initialized_) systems_.back()->Initialize();
        return dynamic_cast<T*>(systems_.back().get());
    }

    template<typename T>
    T* GetSystem() {
        for (auto& sys : systems_) if (auto typed = std::dynamic_pointer_cast<T>(sys)) return typed.get();
        return nullptr;
    }

    void Initialize() {
        if (initialized_) return;
        for (auto& sys : systems_) sys->Initialize();
        initialized_ = true;
    }

    void Shutdown() {
        if (!initialized_ && systems_.empty()) return;
        for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) (*it)->Shutdown();
        systems_.clear();
        initialized_ = false;
    }

    void Update(Seconds dt) { for (auto& sys : systems_) if (sys->IsEnabled()) sys->Update(dt); }
    void FixedUpdate(Seconds dt) { for (auto& sys : systems_) if (sys->IsEnabled()) sys->FixedUpdate(dt); }
    void Render() { for (auto& sys : systems_) if (sys->IsEnabled()) sys->Render(); }

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
    virtual void OnLevelLoaded(const std::string&) {}
    virtual void OnLevelUnloaded(const std::string&) {}

    template<typename T>
    void SetState(const std::string& key, T value) { state_[key] = std::move(value); }

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
    static Engine& GetInstance() { static Engine instance; return instance; }

    bool Initialize(const EngineConfig& config) {
        if (initialized_) return true;

        config_ = config;
        config_.width = std::max(1, config_.width);
        config_.height = std::max(1, config_.height);
        config_.fixedDeltaTime = std::max(0.0001, config_.fixedDeltaTime);
        timeSystem_.SetFixedDeltaTime(config_.fixedDeltaTime);
        Logger::GetInstance().SetLevel(config_.logLevel);

        window_ = std::make_unique<Window>(config_.width, config_.height, config_.title);
        WireWindowEvents();
        instance_ = std::make_unique<VulkanInstance>(config_.enableVulkanValidation);
        surface_ = window_->CreateSurface(instance_->Handle());
        device_ = std::make_unique<VulkanDevice>(instance_->Handle(), surface_);
        renderer_ = std::make_unique<Renderer>(device_->Physical(), device_->Logical(), surface_,
                                                device_->GraphicsQueueFamily(), device_->GraphicsQueue(),
                                                static_cast<uint32_t>(config_.width), static_cast<uint32_t>(config_.height));

        inputSystem_ = systemManager_.AddSystem<InputSystem>();
        eventDispatcher_.AddListener(inputSystem_);
        systemManager_.Initialize();
        initialized_ = true;
        EOA_LOG_INFO("EOA Engine initialized successfully");
        return true;
    }

    void Run() {
        if (!initialized_) return;
        running_ = true;

        while (running_ && window_ && !window_->ShouldClose()) {
            timeSystem_.Tick();
            if (inputSystem_) inputSystem_->BeginFrame();
            ProcessEvents();
            eventDispatcher_.DispatchPending();
            TaskManager::GetInstance().ProcessTasks(timeSystem_.GetTime().deltaTime);

            if (config_.fixedTimestep) {
                while (timeSystem_.ShouldFixedUpdate()) {
                    timeSystem_.ConsumeFixedStep();
                    systemManager_.FixedUpdate(config_.fixedDeltaTime);
                }
            }

            const Seconds dt = timeSystem_.GetTime().deltaTime;
            systemManager_.Update(dt);

            int width = 0, height = 0;
            window_->FramebufferSize(width, height);
            if (renderer_ && width > 0 && height > 0) {
                renderer_->GetWorld().Tick(static_cast<float>(dt));
                renderer_->DrawFrame(static_cast<uint32_t>(width), static_cast<uint32_t>(height), glm::mat4(1.0f));
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
        running_ = false;
        if (device_) vkDeviceWaitIdle(device_->Logical());
        if (gameInstance_) {
            eventDispatcher_.RemoveListener(gameInstance_.get());
            gameInstance_->Shutdown();
            gameInstance_.reset();
        }
        if (inputSystem_) eventDispatcher_.RemoveListener(inputSystem_);
        systemManager_.Shutdown();
        TaskManager::GetInstance().ClearTasks();
        renderer_.reset();
        device_.reset();
        if (surface_ != VK_NULL_HANDLE && instance_) vkDestroySurfaceKHR(instance_->Handle(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
        instance_.reset();
        window_.reset();
        inputSystem_ = nullptr;
        initialized_ = false;
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
        if (gameInstance_) {
            eventDispatcher_.RemoveListener(gameInstance_.get());
            gameInstance_->Shutdown();
        }
        gameInstance_ = std::move(instance);
        if (gameInstance_) {
            gameInstance_->Init();
            eventDispatcher_.AddListener(gameInstance_.get());
        }
    }

    template<typename T>
    T* GetGameInstance() { return dynamic_cast<T*>(gameInstance_.get()); }

    static Engine& Get() { return GetInstance(); }
    static EventDispatcher& Events() { return Get().GetEventDispatcher(); }
    static InputSystem* Input() { return Get().GetInputSystem(); }
    static const Time& Time() { return Get().GetTime(); }

private:
    Engine() = default;
    ~Engine() { Shutdown(); }
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    static int ModifiersFromGlfw(int mods) {
        int result = 0;
        if (mods & GLFW_MOD_SHIFT) result |= static_cast<int>(ModifierKey::Shift);
        if (mods & GLFW_MOD_CONTROL) result |= static_cast<int>(ModifierKey::Control);
        if (mods & GLFW_MOD_ALT) result |= static_cast<int>(ModifierKey::Alt);
        if (mods & GLFW_MOD_SUPER) result |= static_cast<int>(ModifierKey::Super);
        return result;
    }

    void WireWindowEvents() {
        window_->SetKeyCallback([this](int key, int action, int mods) {
            const auto code = static_cast<KeyCode>(key);
            if (action == GLFW_PRESS || action == GLFW_REPEAT) eventDispatcher_.Send<KeyPressedEvent>(code, action == GLFW_REPEAT ? 1 : 0, ModifiersFromGlfw(mods));
            else if (action == GLFW_RELEASE) eventDispatcher_.Send<KeyReleasedEvent>(code);
        });
        window_->SetMouseMoveCallback([this](double x, double y) {
            const float fx = static_cast<float>(x), fy = static_cast<float>(y);
            const float dx = fx - lastMouseX_, dy = fy - lastMouseY_;
            lastMouseX_ = fx; lastMouseY_ = fy;
            eventDispatcher_.Send<MouseMovedEvent>(fx, fy, dx, dy);
        });
        window_->SetMouseScrollCallback([this](double x, double y) {
            eventDispatcher_.Send<MouseScrolledEvent>(static_cast<float>(x), static_cast<float>(y));
        });
        window_->SetMouseButtonCallback([this](int button, int action, int mods) {
            double x = 0.0, y = 0.0;
            glfwGetCursorPos(window_->Handle(), &x, &y);
            if (action == GLFW_PRESS) eventDispatcher_.Send<MouseButtonPressedEvent>(static_cast<MouseButton>(button), static_cast<float>(x), static_cast<float>(y), ModifiersFromGlfw(mods));
            else if (action == GLFW_RELEASE) eventDispatcher_.Send<MouseButtonReleasedEvent>(static_cast<MouseButton>(button), static_cast<float>(x), static_cast<float>(y));
        });
        window_->SetFramebufferResizeCallback([this](int width, int height) {
            eventDispatcher_.SendAsync<WindowResizeEvent>(width, height);
        });
    }

    void ProcessEvents() { if (window_) window_->PollEvents(); }
    void LimitFrameRate() {
        if (config_.targetFPS <= 0) return;
        const double target = 1.0 / static_cast<double>(config_.targetFPS);
        const double elapsed = timeSystem_.GetTime().GetRealDeltaTime();
        if (elapsed < target) TimeSystem::Sleep((target - elapsed) * 1000.0);
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
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
    bool running_ = false;
    bool initialized_ = false;
};

#define gEngine eoa::Engine::Get()
#define gEvents eoa::Engine::Events()
#define gInput eoa::Engine::Input()
#define gTime eoa::Engine::Time()

} // namespace eoa
