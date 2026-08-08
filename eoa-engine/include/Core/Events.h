#pragma once

#include "Core/Platform.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

namespace EOA {

// ============================================
// СИСТЕМА СОБЫТИЙ (Event System)
// ============================================

enum class EventType {
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    ActorSpawned,
    ActorDestroyed,
    ComponentAdded,
    ComponentRemoved,
    SceneLoaded,
    SceneUnloaded,
    ResourceLoaded,
    ResourceUnloaded,
    Custom
};

#define BIT(x) (1 << x)

enum class EventCategory {
    None = 0,
    Window = BIT(0),
    Input = BIT(1),
    Keyboard = BIT(2),
    Mouse = BIT(3),
    Actor = BIT(4),
    Scene = BIT(5),
    Resource = BIT(6)
};

class Event {
public:
    virtual ~Event() = default;
    
    bool IsInCategory(EventCategory category) const {
        return (GetCategory() & category) != EventCategory::None;
    }
    
    virtual EventType GetType() const = 0;
    virtual EventCategory GetCategory() const = 0;
    virtual std::string ToString() const = 0;
    
    bool Handled = false;
};

// Window Events
class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(uint32_t w, uint32_t h) : Width(w), Height(h) {}
    
    uint32_t GetWidth() const { return Width; }
    uint32_t GetHeight() const { return Height; }
    
    EventType GetType() const override { return EventType::WindowResize; }
    EventCategory GetCategory() const override { return EventCategory::Window; }
    std::string ToString() const override { 
        return "WindowResizeEvent: " + std::to_string(Width) + ", " + std::to_string(Height); 
    }
    
private:
    uint32_t Width, Height;
};

class WindowCloseEvent : public Event {
public:
    EventType GetType() const override { return EventType::WindowClose; }
    EventCategory GetCategory() const override { return EventCategory::Window; }
    std::string ToString() const override { return "WindowCloseEvent"; }
};

// Keyboard Events
enum class KeyCode : uint16_t {
    Unknown = 0,
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91, Backslash = 92, RightBracket = 93,
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper
};

class KeyPressedEvent : public Event {
public:
    KeyPressedEvent(KeyCode key, bool repeat = false) : Key(key), IsRepeat(repeat) {}
    
    KeyCode GetKeyCode() const { return Key; }
    bool IsKeyPressedRepeatedly() const { return IsRepeat; }
    
    EventType GetType() const override { return EventType::KeyPressed; }
    EventCategory GetCategory() const override { 
        return static_cast<EventCategory>(static_cast<int>(EventCategory::Input) | 
                                          static_cast<int>(EventCategory::Keyboard)); 
    }
    std::string ToString() const override { 
        return "KeyPressedEvent: " + std::to_string(static_cast<int>(Key)); 
    }
    
private:
    KeyCode Key;
    bool IsRepeat;
};

class KeyReleasedEvent : public Event {
public:
    KeyReleasedEvent(KeyCode key) : Key(key) {}
    
    KeyCode GetKeyCode() const { return Key; }
    
    EventType GetType() const override { return EventType::KeyReleased; }
    EventCategory GetCategory() const override { 
        return static_cast<EventCategory>(static_cast<int>(EventCategory::Input) | 
                                          static_cast<int>(EventCategory::Keyboard)); 
    }
    std::string ToString() const override { 
        return "KeyReleasedEvent: " + std::to_string(static_cast<int>(Key)); 
    }
    
private:
    KeyCode Key;
};

// Mouse Events
enum class MouseButton : uint8_t {
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    ButtonLast = 7,
    Left = Button0,
    Right = Button1,
    Middle = Button2
};

class MouseButtonPressedEvent : public Event {
public:
    MouseButtonPressedEvent(MouseButton button) : Button(button) {}
    
    MouseButton GetMouseButton() const { return Button; }
    
    EventType GetType() const override { return EventType::MouseButtonPressed; }
    EventCategory GetCategory() const override { 
        return static_cast<EventCategory>(static_cast<int>(EventCategory::Input) | 
                                          static_cast<int>(EventCategory::Mouse)); 
    }
    std::string ToString() const override { 
        return "MouseButtonPressedEvent: " + std::to_string(static_cast<int>(Button)); 
    }
    
private:
    MouseButton Button;
};

class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y) : X(x), Y(y) {}
    
    float GetX() const { return X; }
    float GetY() const { return Y; }
    
    EventType GetType() const override { return EventType::MouseMoved; }
    EventCategory GetCategory() const override { 
        return static_cast<EventCategory>(static_cast<int>(EventCategory::Input) | 
                                          static_cast<int>(EventCategory::Mouse)); 
    }
    std::string ToString() const override { 
        return "MouseMovedEvent: " + std::to_string(X) + ", " + std::to_string(Y); 
    }
    
private:
    float X, Y;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset) : XOffset(xOffset), YOffset(yOffset) {}
    
    float GetXOffset() const { return XOffset; }
    float GetYOffset() const { return YOffset; }
    
    EventType GetType() const override { return EventType::MouseScrolled; }
    EventCategory GetCategory() const override { 
        return static_cast<EventCategory>(static_cast<int>(EventCategory::Input) | 
                                          static_cast<int>(EventCategory::Mouse)); 
    }
    std::string ToString() const override { 
        return "MouseScrolledEvent: " + std::to_string(XOffset) + ", " + std::to_string(YOffset); 
    }
    
private:
    float XOffset, YOffset;
};

// ============================================
// МЕНЕДЖЕР СОБЫТИЙ (Event Dispatcher)
// ============================================

using EventCallbackFn = std::function<void(Event&)>;

class EventDispatcher {
public:
    void Subscribe(EventType type, EventCallbackFn callback) {
        Callbacks[type].push_back(callback);
    }
    
    void Dispatch(Event& event) {
        auto it = Callbacks.find(event.GetType());
        if (it != Callbacks.end()) {
            for (auto& callback : it->second) {
                callback(event);
                if (event.Handled) break;
            }
        }
    }
    
    void DispatchPending() {
        // Обработка очереди событий
        std::vector<Event*> pending;
        {
            std::lock_guard<std::mutex> lock(PendingMutex);
            pending.swap(PendingEvents);
        }
        
        for (auto* event : pending) {
            Dispatch(*event);
            delete event;
        }
    }
    
    void QueueEvent(Event* event) {
        std::lock_guard<std::mutex> lock(PendingMutex);
        PendingEvents.push_back(event);
    }
    
private:
    std::unordered_map<EventType, std::vector<EventCallbackFn>> Callbacks;
    std::vector<Event*> PendingEvents;
    std::mutex PendingMutex;
};

} // namespace EOA
