#pragma once

#include <algorithm>
#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eoa {

enum class EventType {
    None = 0, WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMove,
    KeyPressed, KeyReleased, KeyTyped, MouseButtonPressed, MouseButtonReleased,
    MouseMoved, MouseScrolled, ActorSpawned, ActorDestroyed, ComponentAdded,
    ComponentRemoved, LevelLoaded, LevelUnloaded, ResourceLoaded, ResourceUnloaded, Custom
};

enum class EventCategory {
    None = 0,
    Application = (1 << 0),
    Input = (1 << 1),
    World = (1 << 2),
    Resource = (1 << 3),
    Custom = (1 << 4)
};

class Event {
public:
    virtual ~Event() = default;
    EventType GetType() const { return type_; }
    const std::string& GetName() const { return name_; }
    int GetCategoryFlags() const { return category_; }
    bool IsInCategory(EventCategory cat) const { return (category_ & static_cast<int>(cat)) != 0; }
    bool Handled() const { return handled_; }
    void SetHandled(bool handled) { handled_ = handled; }
    virtual std::string ToString() const { return name_; }

protected:
    Event(EventType type, const std::string& name, int category)
        : type_(type), name_(name), category_(category) {}
    EventType type_;
    std::string name_;
    int category_;
    bool handled_ = false;
};

class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() : Event(EventType::WindowClose, "WindowClose", static_cast<int>(EventCategory::Application)) {}
    std::string ToString() const override { return "WindowCloseEvent"; }
};

class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(int width, int height)
        : Event(EventType::WindowResize, "WindowResize", static_cast<int>(EventCategory::Application)), width_(width), height_(height) {}
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    std::string ToString() const override { return "WindowResizeEvent(" + std::to_string(width_) + ", " + std::to_string(height_) + ")"; }
private:
    int width_, height_;
};

class WindowFocusEvent : public Event {
public:
    WindowFocusEvent() : Event(EventType::WindowFocus, "WindowFocus", static_cast<int>(EventCategory::Application)) {}
};

class WindowLostFocusEvent : public Event {
public:
    WindowLostFocusEvent() : Event(EventType::WindowLostFocus, "WindowLostFocus", static_cast<int>(EventCategory::Application)) {}
};

class WindowMoveEvent : public Event {
public:
    WindowMoveEvent(int x, int y)
        : Event(EventType::WindowMove, "WindowMove", static_cast<int>(EventCategory::Application)), x_(x), y_(y) {}
    int GetX() const { return x_; }
    int GetY() const { return y_; }
private:
    int x_, y_;
};

enum class KeyCode {
    Unknown = -1, Space = 32, Apostrophe = 39, Comma = 44, Minus = 45, Period = 46, Slash = 47,
    D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9, Semicolon = 59, Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91, Backslash = 92, RightBracket = 93, GraveAccent = 96,
    Escape = 256, Enter = 257, Tab = 258, Backspace = 259, Insert = 260, Delete = 261,
    Right = 262, Left = 263, Down = 264, Up = 265, PageUp = 266, PageDown = 267,
    Home = 268, End = 269, CapsLock = 280, ScrollLock = 281, NumLock = 282, PrintScreen = 283,
    Pause = 284, F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340, LeftControl = 341, LeftAlt = 342, LeftSuper = 343,
    RightShift = 344, RightControl = 345, RightAlt = 346, RightSuper = 347, Menu = 348
};

enum class ModifierKey {
    Shift = (1 << 0), Control = (1 << 1), Alt = (1 << 2), Super = (1 << 3)
};

class KeyPressedEvent : public Event {
public:
    KeyPressedEvent(KeyCode keycode, int repeatCount = 0, int modifiers = 0)
        : Event(EventType::KeyPressed, "KeyPressed", static_cast<int>(EventCategory::Input)), keycode_(keycode), repeatCount_(repeatCount), modifiers_(modifiers) {}
    KeyCode GetKeyCode() const { return keycode_; }
    int GetRepeatCount() const { return repeatCount_; }
    int GetModifiers() const { return modifiers_; }
    bool IsShiftPressed() const { return modifiers_ & static_cast<int>(ModifierKey::Shift); }
    bool IsControlPressed() const { return modifiers_ & static_cast<int>(ModifierKey::Control); }
    bool IsAltPressed() const { return modifiers_ & static_cast<int>(ModifierKey::Alt); }
    std::string ToString() const override { return "KeyPressedEvent(" + std::to_string(static_cast<int>(keycode_)) + ")"; }
private:
    KeyCode keycode_;
    int repeatCount_;
    int modifiers_;
};

class KeyReleasedEvent : public Event {
public:
    explicit KeyReleasedEvent(KeyCode keycode) : Event(EventType::KeyReleased, "KeyReleased", static_cast<int>(EventCategory::Input)), keycode_(keycode) {}
    KeyCode GetKeyCode() const { return keycode_; }
    std::string ToString() const override { return "KeyReleasedEvent(" + std::to_string(static_cast<int>(keycode_)) + ")"; }
private:
    KeyCode keycode_;
};

class KeyTypedEvent : public Event {
public:
    explicit KeyTypedEvent(unsigned int unicode) : Event(EventType::KeyTyped, "KeyTyped", static_cast<int>(EventCategory::Input)), unicode_(unicode) {}
    unsigned int GetUnicode() const { return unicode_; }
    char AsChar() const { return static_cast<char>(unicode_); }
private:
    unsigned int unicode_;
};

enum class MouseButton { Button0 = 0, Button1, Button2, Button3, Button4, Button5, Button6, Button7, Last = Button7, Left = Button0, Right = Button1, Middle = Button2 };

class MouseButtonPressedEvent : public Event {
public:
    MouseButtonPressedEvent(MouseButton button, float x, float y, int modifiers = 0)
        : Event(EventType::MouseButtonPressed, "MouseButtonPressed", static_cast<int>(EventCategory::Input)), button_(button), x_(x), y_(y), modifiers_(modifiers) {}
    MouseButton GetButton() const { return button_; }
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    int GetModifiers() const { return modifiers_; }
    std::string ToString() const override { return "MouseButtonPressedEvent(" + std::to_string(static_cast<int>(button_)) + ", " + std::to_string(x_) + ", " + std::to_string(y_) + ")"; }
private:
    MouseButton button_; float x_, y_; int modifiers_;
};

class MouseButtonReleasedEvent : public Event {
public:
    MouseButtonReleasedEvent(MouseButton button, float x, float y)
        : Event(EventType::MouseButtonReleased, "MouseButtonReleased", static_cast<int>(EventCategory::Input)), button_(button), x_(x), y_(y) {}
    MouseButton GetButton() const { return button_; }
    float GetX() const { return x_; }
    float GetY() const { return y_; }
private:
    MouseButton button_; float x_, y_;
};

class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y, float deltaX, float deltaY)
        : Event(EventType::MouseMoved, "MouseMoved", static_cast<int>(EventCategory::Input)), x_(x), y_(y), deltaX_(deltaX), deltaY_(deltaY) {}
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetDeltaX() const { return deltaX_; }
    float GetDeltaY() const { return deltaY_; }
    std::string ToString() const override { return "MouseMovedEvent(" + std::to_string(x_) + ", " + std::to_string(y_) + ")"; }
private:
    float x_, y_, deltaX_, deltaY_;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : Event(EventType::MouseScrolled, "MouseScrolled", static_cast<int>(EventCategory::Input)), xOffset_(xOffset), yOffset_(yOffset) {}
    float GetXOffset() const { return xOffset_; }
    float GetYOffset() const { return yOffset_; }
private:
    float xOffset_, yOffset_;
};

class Actor;
class Component;

class ActorSpawnedEvent : public Event {
public:
    explicit ActorSpawnedEvent(Actor* actor) : Event(EventType::ActorSpawned, "ActorSpawned", static_cast<int>(EventCategory::World)), actor_(actor) {}
    Actor* GetActor() const { return actor_; }
private:
    Actor* actor_;
};

class ActorDestroyedEvent : public Event {
public:
    explicit ActorDestroyedEvent(uint64_t actorId) : Event(EventType::ActorDestroyed, "ActorDestroyed", static_cast<int>(EventCategory::World)), actorId_(actorId) {}
    uint64_t GetActorId() const { return actorId_; }
private:
    uint64_t actorId_;
};

class ComponentAddedEvent : public Event {
public:
    ComponentAddedEvent(Actor* actor, Component* component) : Event(EventType::ComponentAdded, "ComponentAdded", static_cast<int>(EventCategory::World)), actor_(actor), component_(component) {}
    Actor* GetActor() const { return actor_; }
    Component* GetComponent() const { return component_; }
private:
    Actor* actor_; Component* component_;
};

class ComponentRemovedEvent : public Event {
public:
    ComponentRemovedEvent(Actor* actor, Component* component) : Event(EventType::ComponentRemoved, "ComponentRemoved", static_cast<int>(EventCategory::World)), actor_(actor), component_(component) {}
    Actor* GetActor() const { return actor_; }
    Component* GetComponent() const { return component_; }
private:
    Actor* actor_; Component* component_;
};

class LevelLoadedEvent : public Event {
public:
    explicit LevelLoadedEvent(const std::string& levelName) : Event(EventType::LevelLoaded, "LevelLoaded", static_cast<int>(EventCategory::World)), levelName_(levelName) {}
    const std::string& GetLevelName() const { return levelName_; }
private:
    std::string levelName_;
};

class LevelUnloadedEvent : public Event {
public:
    explicit LevelUnloadedEvent(const std::string& levelName) : Event(EventType::LevelUnloaded, "LevelUnloaded", static_cast<int>(EventCategory::World)), levelName_(levelName) {}
    const std::string& GetLevelName() const { return levelName_; }
private:
    std::string levelName_;
};

class ResourceLoadedEvent : public Event {
public:
    ResourceLoadedEvent(const std::string& resourceName, void* resource) : Event(EventType::ResourceLoaded, "ResourceLoaded", static_cast<int>(EventCategory::Resource)), resourceName_(resourceName), resource_(resource) {}
    const std::string& GetResourceName() const { return resourceName_; }
    void* GetResource() const { return resource_; }
private:
    std::string resourceName_; void* resource_;
};

class ResourceUnloadedEvent : public Event {
public:
    explicit ResourceUnloadedEvent(const std::string& resourceName) : Event(EventType::ResourceUnloaded, "ResourceUnloaded", static_cast<int>(EventCategory::Resource)), resourceName_(resourceName) {}
    const std::string& GetResourceName() const { return resourceName_; }
private:
    std::string resourceName_;
};

class CustomEvent : public Event {
public:
    CustomEvent(const std::string& name, std::any data = nullptr) : Event(EventType::Custom, name, static_cast<int>(EventCategory::Custom)), data_(std::move(data)) {}
    template<typename T> T* GetData() { return std::any_cast<T>(&data_); }
    template<typename T> const T* GetData() const { return std::any_cast<T>(&data_); }
private:
    std::any data_;
};

using EventCallbackFn = std::function<void(Event&)>;

class IEventListener {
public:
    virtual ~IEventListener() = default;
    virtual void OnEvent(Event& event) = 0;
};

class EventDispatcher {
public:
    EventDispatcher() = default;

    void AddListener(IEventListener* listener, EventCategory categories = EventCategory::None) {
        if (!listener) return;
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back({listener, categories});
    }

    void RemoveListener(IEventListener* listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.erase(std::remove_if(listeners_.begin(), listeners_.end(),
            [listener](const ListenerInfo& info) { return info.listener == listener; }), listeners_.end());
    }

    void Dispatch(Event& event) {
        std::vector<ListenerInfo> listeners;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            listeners = listeners_;
        }
        for (const auto& info : listeners) {
            if (event.Handled()) break;
            if (info.listener && (info.categories == EventCategory::None || event.IsInCategory(info.categories))) {
                info.listener->OnEvent(event);
            }
        }
    }

    void QueueEvent(std::shared_ptr<Event> event) {
        if (!event) return;
        std::lock_guard<std::mutex> lock(mutex_);
        eventQueue_.push(std::move(event));
    }

    void DispatchPending() {
        std::queue<std::shared_ptr<Event>> localQueue;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(localQueue, eventQueue_);
        }
        while (!localQueue.empty()) {
            auto event = std::move(localQueue.front());
            localQueue.pop();
            if (event) Dispatch(*event);
        }
    }

    template<typename EventType, typename... Args>
    void Send(Args&&... args) {
        auto event = std::make_shared<EventType>(std::forward<Args>(args)...);
        Dispatch(*event);
    }

    template<typename EventType, typename... Args>
    void SendAsync(Args&&... args) {
        QueueEvent(std::make_shared<EventType>(std::forward<Args>(args)...));
    }

private:
    struct ListenerInfo { IEventListener* listener; EventCategory categories; };
    std::vector<ListenerInfo> listeners_;
    std::queue<std::shared_ptr<Event>> eventQueue_;
    std::mutex mutex_;
};

} // namespace eoa
