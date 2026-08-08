#pragma once

#include "core/eoa_core.h"
#include <string>
#include <functional>

// Forward declare GLFW
struct GLFWwindow;

namespace EOA
{
    /**
     * @brief Конфигурация окна
     */
    struct WindowConfig
    {
        std::string Title = "EOA Engine";
        uint32 Width = 1920;
        uint32 Height = 1080;
        bool Fullscreen = false;
        bool VSync = true;
        bool Resizable = true;
    };

    /**
     * @brief Обёртка над GLFW-окном (обновлённая версия)
     * Один экземпляр = одно окно приложения.
     */
    class EOA_API Window
    {
    public:
        Window();
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        /**
         * @brief Инициализация окна с заданной конфигурацией
         */
        bool Initialize(const WindowConfig& config);

        /**
         * @brief Проверка флага закрытия окна
         */
        bool ShouldClose() const;

        /**
         * @brief Опрос событий окна
         */
        void PollEvents() const;

        /**
         * @brief Получить дескриптор GLFW окна
         */
        GLFWwindow* Handle() const { return handle_; }

        /**
         * @brief Получить ширину окна
         */
        int Width() const { return width_; }

        /**
         * @brief Получить высоту окна
         */
        int Height() const { return height_; }

        /**
         * @brief Получить текущий размер framebuffer'а
         */
        void FramebufferSize(int& outWidth, int& outHeight) const;

        /**
         * @brief Блокировка пока окно свёрнуто
         */
        void WaitWhileMinimized() const;

        // --- Callbacks для ввода ---
        using KeyCallback = std::function<void(int, int, int)>;
        using MouseMoveCallback = std::function<void(double, double)>;
        using MouseScrollCallback = std::function<void(double, double)>;
        using MouseButtonCallback = std::function<void(int, int, int)>;

        void SetKeyCallback(KeyCallback callback) { m_KeyCallback = callback; }
        void SetMouseMoveCallback(MouseMoveCallback callback) { m_MouseMoveCallback = callback; }
        void SetMouseScrollCallback(MouseScrollCallback callback) { m_MouseScrollCallback = callback; }
        void SetMouseButtonCallback(MouseButtonCallback callback) { m_MouseButtonCallback = callback; }

    private:
        static void KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseMoveCallbackStatic(GLFWwindow* window, double xpos, double ypos);
        static void MouseScrollCallbackStatic(GLFWwindow* window, double xoffset, double yoffset);
        static void MouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods);

        GLFWwindow* handle_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        WindowConfig m_Config;

        KeyCallback m_KeyCallback;
        MouseMoveCallback m_MouseMoveCallback;
        MouseScrollCallback m_MouseScrollCallback;
        MouseButtonCallback m_MouseButtonCallback;
    };

} // namespace EOA
