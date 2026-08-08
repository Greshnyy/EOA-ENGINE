#pragma once

#include "events/EventSystem.h"
#include <unordered_map>
#include <mutex>

namespace eoa {

// ============================================================================
// СИСТЕМА ВВОДА
// ============================================================================

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

class InputSystem : public IEventListener {
public:
    InputSystem() = default;
    
    void OnEvent(Event& event) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        switch (event.GetType()) {
            case EventType::KeyPressed: {
                auto& e = static_cast<KeyPressedEvent&>(event);
                state_.keys[e.GetKeyCode()] = true;
                state_.modifiers = e.GetModifiers();
                break;
            }
            case EventType::KeyReleased: {
                auto& e = static_cast<KeyReleasedEvent&>(event);
                state_.keys[e.GetKeyCode()] = false;
                break;
            }
            case EventType::MouseButtonPressed: {
                auto& e = static_cast<MouseButtonPressedEvent&>(event);
                state_.mouseButtons[e.GetButton()] = true;
                state_.mouseX = e.GetX();
                state_.mouseY = e.GetY();
                break;
            }
            case EventType::MouseButtonReleased: {
                auto& e = static_cast<MouseButtonReleasedEvent&>(event);
                state_.mouseButtons[e.GetButton()] = false;
                state_.mouseX = e.GetX();
                state_.mouseY = e.GetY();
                break;
            }
            case EventType::MouseMoved: {
                auto& e = static_cast<MouseMovedEvent&>(event);
                state_.mouseX = e.GetX();
                state_.mouseY = e.GetY();
                state_.mouseDeltaX = e.GetDeltaX();
                state_.mouseDeltaY = e.GetDeltaY();
                break;
            }
            case EventType::MouseScrolled: {
                auto& e = static_cast<MouseScrolledEvent&>(event);
                state_.scrollX = e.GetXOffset();
                state_.scrollY = e.GetYOffset();
                break;
            }
            default:
                break;
        }
    }
    
    // Обновление состояния (вызывать в начале каждого кадра)
    void BeginFrame() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.keysPrev = state_.keys;
        state_.mouseButtonsPrev = state_.mouseButtons;
        state_.scrollX = 0.0f;
        state_.scrollY = 0.0f;
        state_.mouseDeltaX = 0.0f;
        state_.mouseDeltaY = 0.0f;
    }
    
    // Клавиатура
    bool IsKeyDown(KeyCode key) const {
        auto it = state_.keys.find(key);
        return it != state_.keys.end() && it->second;
    }
    
    bool IsKeyPressed(KeyCode key) const {
        bool curr = IsKeyDown(key);
        bool prev = false;
        auto it = state_.keysPrev.find(key);
        if (it != state_.keysPrev.end()) prev = it->second;
        return curr && !prev;
    }
    
    bool IsKeyReleased(KeyCode key) const {
        bool curr = IsKeyDown(key);
        bool prev = false;
        auto it = state_.keysPrev.find(key);
        if (it != state_.keysPrev.end()) prev = it->second;
        return !curr && prev;
    }
    
    // Мышь
    bool IsMouseButtonDown(MouseButton button) const {
        auto it = state_.mouseButtons.find(button);
        return it != state_.mouseButtons.end() && it->second;
    }
    
    bool IsMouseButtonPressed(MouseButton button) const {
        bool curr = IsMouseButtonDown(button);
        bool prev = false;
        auto it = state_.mouseButtonsPrev.find(button);
        if (it != state_.mouseButtonsPrev.end()) prev = it->second;
        return curr && !prev;
    }
    
    bool IsMouseButtonReleased(MouseButton button) const {
        bool curr = IsMouseButtonDown(button);
        bool prev = false;
        auto it = state_.mouseButtonsPrev.find(button);
        if (it != state_.mouseButtonsPrev.end()) prev = it->second;
        return !curr && prev;
    }
    
    float GetMouseX() const { return state_.mouseX; }
    float GetMouseY() const { return state_.mouseY; }
    float GetMouseDeltaX() const { return state_.mouseDeltaX; }
    float GetMouseDeltaY() const { return state_.mouseDeltaY; }
    float GetScrollX() const { return state_.scrollX; }
    float GetScrollY() const { return state_.scrollY; }
    
    // Модификаторы
    bool IsShiftPressed() const { return state_.modifiers & static_cast<int>(ModifierKey::Shift); }
    bool IsControlPressed() const { return state_.modifiers & static_cast<int>(ModifierKey::Control); }
    bool IsAltPressed() const { return state_.modifiers & static_cast<int>(ModifierKey::Alt); }
    
    // Сброс
    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = InputState();
    }

private:
    mutable std::mutex mutex_;
    InputState state_;
};

} // namespace eoa
