#pragma once

#include "Core/Types.h"
#include "Core/Events.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace EOA {

// ============================================
// СИСТЕМА ВВОДА (Input System)
// ============================================

class InputSystem {
public:
    static InputSystem* GetInstance() {
        static InputSystem instance;
        return &instance;
    }
    
    // Состояние клавиш
    bool IsKeyPressed(KeyCode key) const {
        auto it = KeyStates.find(key);
        return it != KeyStates.end() && it->second;
    }
    
    bool IsKeyJustPressed(KeyCode key) const {
        return KeyJustPressed.count(key) > 0;
    }
    
    bool IsKeyJustReleased(KeyCode key) const {
        return KeyJustReleased.count(key) > 0;
    }
    
    // Состояние мыши
    bool IsMouseButtonPressed(MouseButton button) const {
        auto it = MouseButtonStates.find(button);
        return it != MouseButtonStates.end() && it->second;
    }
    
    bool IsMouseButtonJustPressed(MouseButton button) const {
        return MouseJustPressed.count(button) > 0;
    }
    
    float GetMouseX() const { return MouseX; }
    float GetMouseY() const { return MouseY; }
    float GetMouseDeltaX() const { return MouseDeltaX; }
    float GetMouseDeltaY() const { return MouseDeltaY; }
    float GetScrollX() const { return ScrollX; }
    float GetScrollY() const { return ScrollY; }
    
    // Обработка событий
    void OnEvent(Event& event) {
        if (event.Handled) return;
        
        if (auto* keyEvent = dynamic_cast<KeyPressedEvent*>(&event)) {
            KeyStates[keyEvent->GetKeyCode()] = true;
            if (!keyEvent->IsKeyPressedRepeatedly()) {
                KeyJustPressed.insert(keyEvent->GetKeyCode());
            }
            event.Handled = true;
        }
        else if (auto* keyEvent = dynamic_cast<KeyReleasedEvent*>(&event)) {
            KeyStates[keyEvent->GetKeyCode()] = false;
            KeyJustReleased.insert(keyEvent->GetKeyCode());
            event.Handled = true;
        }
        else if (auto* mouseEvent = dynamic_cast<MouseButtonPressedEvent*>(&event)) {
            MouseButtonStates[mouseEvent->GetMouseButton()] = true;
            MouseJustPressed.insert(mouseEvent->GetMouseButton());
            event.Handled = true;
        }
        else if (auto* mouseEvent = dynamic_cast<MouseButtonPressedEvent*>(&event)) {
            MouseButtonStates[mouseEvent->GetMouseButton()] = false;
            MouseJustReleased.insert(mouseEvent->GetMouseButton());
            event.Handled = true;
        }
        else if (auto* mouseEvent = dynamic_cast<MouseMovedEvent*>(&event)) {
            MouseDeltaX = mouseEvent->GetX() - MouseX;
            MouseDeltaY = mouseEvent->GetY() - MouseY;
            MouseX = mouseEvent->GetX();
            MouseY = mouseEvent->GetY();
            event.Handled = true;
        }
        else if (auto* scrollEvent = dynamic_cast<MouseScrolledEvent*>(&event)) {
            ScrollX = scrollEvent->GetXOffset();
            ScrollY = scrollEvent->GetYOffset();
            event.Handled = true;
        }
    }
    
    // Обновление (сброс just-состояний)
    void Update() {
        KeyJustPressed.clear();
        KeyJustReleased.clear();
        MouseJustPressed.clear();
        MouseJustReleased.clear();
        MouseDeltaX = 0.0f;
        MouseDeltaY = 0.0f;
        ScrollX = 0.0f;
        ScrollY = 0.0f;
    }
    
private:
    InputSystem() = default;
    
    std::unordered_map<KeyCode, bool> KeyStates;
    std::unordered_set<KeyCode> KeyJustPressed;
    std::unordered_set<KeyCode> KeyJustReleased;
    
    std::unordered_map<MouseButton, bool> MouseButtonStates;
    std::unordered_set<MouseButton> MouseJustPressed;
    std::unordered_set<MouseButton> MouseJustReleased;
    
    float MouseX = 0.0f, MouseY = 0.0f;
    float MouseDeltaX = 0.0f, MouseDeltaY = 0.0f;
    float ScrollX = 0.0f, ScrollY = 0.0f;
};

#define EOA_INPUT EOA::InputSystem::GetInstance()

} // namespace EOA
