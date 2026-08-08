#pragma once

#include "platform/window.h"  // eoa::Window (старый)
#include "input/eoa_input_system.h"  // EOA::InputSystem

namespace EOA
{
    /**
     * @brief Адаптер для интеграции старого eoa::Window с новой системой ввода EOA
     */
    class WindowAdapter
    {
    public:
        WindowAdapter() = default;
        
        void Initialize(uint32 width, uint32 height, const std::string& title, InputSystem* inputSystem)
        {
            m_Window = std::make_unique<eoa::Window>(width, height, title);
            m_InputSystem = inputSystem;
            
            // Подключаем callback'и окна к системе ввода
            m_Window->SetKeyCallback([this](int key, int action, int mods) {
                if (m_InputSystem) {
                    m_InputSystem->OnKeyPress(key, action, mods);
                }
            });
            
            m_Window->SetMouseMoveCallback([this](double x, double y) {
                if (m_InputSystem) {
                    m_InputSystem->OnMouseMove(x, y);
                }
            });
            
            m_Window->SetMouseScrollCallback([this](double x, double y) {
                if (m_InputSystem) {
                    m_InputSystem->OnMouseScroll(x, y);
                }
            });
            
            m_Window->SetMouseButtonCallback([this](int button, int action, int mods) {
                if (m_InputSystem) {
                    m_InputSystem->OnMouseButton(button, action, mods);
                }
            });
        }
        
        eoa::Window* GetWindow() const { return m_Window.get(); }
        
        bool ShouldClose() const { return m_Window->ShouldClose(); }
        void PollEvents() const { m_Window->PollEvents(); }
        void FramebufferSize(int& w, int& h) const { m_Window->FramebufferSize(w, h); }
        void WaitWhileMinimized() const { m_Window->WaitWhileMinimized(); }
        
    private:
        std::unique_ptr<eoa::Window> m_Window;
        InputSystem* m_InputSystem = nullptr;
    };
}
