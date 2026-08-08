#include "input/eoa_input_system.h"
#include "log.h"

// Временная заглушка для GLFW - нужно будет заменить на реальную интеграцию
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2

namespace EOA
{
    InputSystem::InputSystem()
    {
        LOG_INFO("InputSystem created");
    }

    InputSystem::~InputSystem()
    {
        LOG_INFO("InputSystem destroyed");
    }

    void InputSystem::Initialize(Window& window)
    {
        m_Window = &window;
        LOG_INFO("InputSystem initialized");
        
        // Здесь должна быть регистрация callback'ов с окном
        // window.SetKeyPressCallback([this](int key, int action, int mods) {
        //     OnKeyPress(key, action, mods);
        // });
        // и т.д.
    }

    void InputSystem::Update()
    {
        // Сброс состояний Pressed/Released в конец кадра
        for (auto& [key, state] : m_Keys)
        {
            if (state == InputState::Pressed)
                state = InputState::Held;
            else if (state == InputState::Released)
                state = InputState::None;
        }

        for (auto& [button, state] : m_MouseButtons)
        {
            if (state == InputState::Pressed)
                state = InputState::Held;
            else if (state == InputState::Released)
                state = InputState::None;
        }

        // Расчет дельты мыши
        m_MouseDelta = m_MousePos - m_LastMousePos;
        m_LastMousePos = m_MousePos;

        // Сброс прокрутки после обработки
        m_MouseScroll = {0.0f, 0.0f};
    }

    bool InputSystem::IsKeyPressed(KeyCode key) const
    {
        auto it = m_Keys.find(static_cast<int32>(key));
        return it != m_Keys.end() && it->second == InputState::Pressed;
    }

    bool InputSystem::IsKeyDown(KeyCode key) const
    {
        auto it = m_Keys.find(static_cast<int32>(key));
        return it != m_Keys.end() && 
               (it->second == InputState::Held || it->second == InputState::Pressed);
    }

    bool InputSystem::IsKeyReleased(KeyCode key) const
    {
        auto it = m_Keys.find(static_cast<int32>(key));
        return it != m_Keys.end() && it->second == InputState::Released;
    }

    bool InputSystem::IsMouseButtonPressed(MouseButton button) const
    {
        auto it = m_MouseButtons.find(static_cast<int32>(button));
        return it != m_MouseButtons.end() && it->second == InputState::Pressed;
    }

    bool InputSystem::IsMouseButtonDown(MouseButton button) const
    {
        auto it = m_MouseButtons.find(static_cast<int32>(button));
        return it != m_MouseButtons.end() && 
               (it->second == InputState::Held || it->second == InputState::Pressed);
    }

    bool InputSystem::IsMouseButtonReleased(MouseButton button) const
    {
        auto it = m_MouseButtons.find(static_cast<int32>(button));
        return it != m_MouseButtons.end() && it->second == InputState::Released;
    }

    void InputSystem::OnKeyPress(int32 key, int32 action, int32 mods)
    {
        if (action == GLFW_PRESS)
        {
            m_Keys[key] = InputState::Pressed;
            LOG_DEBUG("Key pressed: {}", key);
        }
        else if (action == GLFW_RELEASE)
        {
            m_Keys[key] = InputState::Released;
            LOG_DEBUG("Key released: {}", key);
        }
    }

    void InputSystem::OnMouseMove(double xpos, double ypos)
    {
        m_MousePos = {static_cast<float>(xpos), static_cast<float>(ypos)};
    }

    void InputSystem::OnMouseScroll(double xoffset, double yoffset)
    {
        m_MouseScroll = {static_cast<float>(xoffset), static_cast<float>(yoffset)};
    }

    void InputSystem::OnMouseButton(int32 button, int32 action, int32 mods)
    {
        if (action == GLFW_PRESS)
        {
            m_MouseButtons[button] = InputState::Pressed;
            LOG_DEBUG("Mouse button pressed: {}", button);
        }
        else if (action == GLFW_RELEASE)
        {
            m_MouseButtons[button] = InputState::Released;
            LOG_DEBUG("Mouse button released: {}", button);
        }
    }

} // namespace EOA
