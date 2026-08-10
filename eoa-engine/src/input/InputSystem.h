#pragma once

#include "core/Systems.h"
#include "events/EventSystem.h"
#include <unordered_map>
#include <mutex>

namespace eoa {

struct InputState {
    std::unordered_map<KeyCode, bool> keys;
    std::unordered_map<KeyCode, bool> keysPrev;
    std::unordered_map<MouseButton, bool> mouseButtons;
    std::unordered_map<MouseButton, bool> mouseButtonsPrev;
    float mouseX = 0.0f, mouseY = 0.0f;
    float mouseDeltaX = 0.0f, mouseDeltaY = 0.0f;
    float scrollX = 0.0f, scrollY = 0.0f;
    int modifiers = 0;
};

class InputSystem : public ISystem, public IEventListener {
public:
    InputSystem() = default;
    void Initialize() override { Reset(); }
    void Shutdown() override { Reset(); }

    void OnEvent(Event& event) override {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (event.GetType()) {
            case EventType::KeyPressed: { auto& e=static_cast<KeyPressedEvent&>(event); state_.keys[e.GetKeyCode()]=true; state_.modifiers=e.GetModifiers(); break; }
            case EventType::KeyReleased: { auto& e=static_cast<KeyReleasedEvent&>(event); state_.keys[e.GetKeyCode()]=false; break; }
            case EventType::MouseButtonPressed: { auto& e=static_cast<MouseButtonPressedEvent&>(event); state_.mouseButtons[e.GetButton()]=true; state_.mouseX=e.GetX(); state_.mouseY=e.GetY(); break; }
            case EventType::MouseButtonReleased: { auto& e=static_cast<MouseButtonReleasedEvent&>(event); state_.mouseButtons[e.GetButton()]=false; state_.mouseX=e.GetX(); state_.mouseY=e.GetY(); break; }
            case EventType::MouseMoved: { auto& e=static_cast<MouseMovedEvent&>(event); state_.mouseX=e.GetX(); state_.mouseY=e.GetY(); state_.mouseDeltaX=e.GetDeltaX(); state_.mouseDeltaY=e.GetDeltaY(); break; }
            case EventType::MouseScrolled: { auto& e=static_cast<MouseScrolledEvent&>(event); state_.scrollX=e.GetXOffset(); state_.scrollY=e.GetYOffset(); break; }
            default: break;
        }
    }

    void BeginFrame() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.keysPrev=state_.keys; state_.mouseButtonsPrev=state_.mouseButtons;
        state_.scrollX=state_.scrollY=0.0f; state_.mouseDeltaX=state_.mouseDeltaY=0.0f;
    }
    bool IsKeyDown(KeyCode key) const { std::lock_guard<std::mutex> l(mutex_); auto i=state_.keys.find(key); return i!=state_.keys.end()&&i->second; }
    bool IsKeyPressed(KeyCode key) const { std::lock_guard<std::mutex> l(mutex_); auto c=state_.keys.find(key),p=state_.keysPrev.find(key); return c!=state_.keys.end()&&c->second&&(p==state_.keysPrev.end()||!p->second); }
    bool IsKeyReleased(KeyCode key) const { std::lock_guard<std::mutex> l(mutex_); auto c=state_.keys.find(key),p=state_.keysPrev.find(key); return (c==state_.keys.end()||!c->second)&&p!=state_.keysPrev.end()&&p->second; }
    bool IsMouseButtonDown(MouseButton button) const { std::lock_guard<std::mutex> l(mutex_); auto i=state_.mouseButtons.find(button); return i!=state_.mouseButtons.end()&&i->second; }
    bool IsMouseButtonPressed(MouseButton button) const { std::lock_guard<std::mutex> l(mutex_); auto c=state_.mouseButtons.find(button),p=state_.mouseButtonsPrev.find(button); return c!=state_.mouseButtons.end()&&c->second&&(p==state_.mouseButtonsPrev.end()||!p->second); }
    bool IsMouseButtonReleased(MouseButton button) const { std::lock_guard<std::mutex> l(mutex_); auto c=state_.mouseButtons.find(button),p=state_.mouseButtonsPrev.find(button); return (c==state_.mouseButtons.end()||!c->second)&&p!=state_.mouseButtonsPrev.end()&&p->second; }
    float GetMouseX() const { std::lock_guard<std::mutex> l(mutex_); return state_.mouseX; }
    float GetMouseY() const { std::lock_guard<std::mutex> l(mutex_); return state_.mouseY; }
    float GetMouseDeltaX() const { std::lock_guard<std::mutex> l(mutex_); return state_.mouseDeltaX; }
    float GetMouseDeltaY() const { std::lock_guard<std::mutex> l(mutex_); return state_.mouseDeltaY; }
    float GetScrollX() const { std::lock_guard<std::mutex> l(mutex_); return state_.scrollX; }
    float GetScrollY() const { std::lock_guard<std::mutex> l(mutex_); return state_.scrollY; }
    bool IsShiftPressed() const { std::lock_guard<std::mutex> l(mutex_); return state_.modifiers & static_cast<int>(ModifierKey::Shift); }
    bool IsControlPressed() const { std::lock_guard<std::mutex> l(mutex_); return state_.modifiers & static_cast<int>(ModifierKey::Control); }
    bool IsAltPressed() const { std::lock_guard<std::mutex> l(mutex_); return state_.modifiers & static_cast<int>(ModifierKey::Alt); }
    void Reset() { std::lock_guard<std::mutex> l(mutex_); state_=InputState(); }

private:
    mutable std::mutex mutex_;
    InputState state_;
};

} // namespace eoa
