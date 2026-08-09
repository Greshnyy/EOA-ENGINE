#pragma once

#include "core/eoa_core.h"
#include <unordered_map>
#include <functional>

// Forward declaration
namespace eoa { class Window; }

namespace EOA
{
    /**
     * @brief Коды клавиш (аналог GLFW key codes)
     */
    enum class KeyCode : int32
    {
        Unknown = -1,
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        Semicolon = 59,
        Equal = 61,
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348
    };

    /**
     * @brief Коды кнопок мыши
     */
    enum class MouseButton : int32
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4
    };

    /**
     * @brief Состояние кнопки/клавиши
     */
    enum class InputState : uint8
    {
        None = 0,       // Не нажата
        Pressed,        // Нажата в этом кадре
        Held,           // Удерживается
        Released        // Отпущена в этом кадре
    };

    /**
     * @brief Система ввода (аналог UInputSystem в UE)
     * Обрабатывает ввод с клавиатуры, мыши и геймпадов.
     */
    class EOA_API InputSystem
    {
    public:
        InputSystem();
        ~InputSystem();

        /**
         * @brief Инициализация системы ввода
         * @param window Окно для получения событий
         */
        void Initialize(eoa::Window& window);

        /**
         * @brief Обновление состояния ввода (вызывать каждый кадр)
         */
        void Update();

        // --- Клавиатура ---
        
        /**
         * @brief Проверка, нажата ли клавиша в этом кадре
         */
        bool IsKeyPressed(KeyCode key) const;

        /**
         * @brief Проверка, удерживается ли клавиша
         */
        bool IsKeyDown(KeyCode key) const;

        /**
         * @brief Проверка, отпущена ли клавиша в этом кадре
         */
        bool IsKeyReleased(KeyCode key) const;

        // --- Мышь ---
        
        /**
         * @brief Проверка, нажата ли кнопка мыши в этом кадре
         */
        bool IsMouseButtonPressed(MouseButton button) const;

        /**
         * @brief Проверка, удерживается ли кнопка мыши
         */
        bool IsMouseButtonDown(MouseButton button) const;

        /**
         * @brief Проверка, отпущена ли кнопка мыши в этом кадре
         */
        bool IsMouseButtonReleased(MouseButton button) const;

        /**
         * @brief Получить позицию курсора мыши
         */
        glm::vec2 GetMousePosition() const { return m_MousePos; }

        /**
         * @brief Получить смещение мыши с прошлого кадра
         */
        glm::vec2 GetMouseDelta() const { return m_MouseDelta; }

        /**
         * @brief Получить прокрутку колесика мыши
         */
        glm::vec2 GetMouseScroll() const { return m_MouseScroll; }

        // --- Callbacks для интеграции с окном ---
        void OnKeyPress(int32 key, int32 action, int32 mods);
        void OnMouseMove(double xpos, double ypos);
        void OnMouseScroll(double xoffset, double yoffset);
        void OnMouseButton(int32 button, int32 action, int32 mods);

    private:
        std::unordered_map<int32, InputState> m_Keys;
        std::unordered_map<int32, InputState> m_MouseButtons;
        
        glm::vec2 m_MousePos = {0.0f, 0.0f};
        glm::vec2 m_MouseDelta = {0.0f, 0.0f};
        glm::vec2 m_MouseScroll = {0.0f, 0.0f};
        glm::vec2 m_LastMousePos = {0.0f, 0.0f};
        
        eoa::Window* m_Window = nullptr;
    };

} // namespace EOA
