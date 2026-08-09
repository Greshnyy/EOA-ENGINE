#pragma once

#include "core/eoa_core.h"
#include "utils/eoa_timer.h"
#include "input/eoa_input_system.h"
#include "core/world.h"
#include "core/eoa_game_instance.h"
#include "core/eoa_window_adapter.h"

// Forward declare
namespace eoa { class Window; }
class Renderer;
namespace eoa { class World; }

namespace EOA
{
    /**
     * @brief Конфигурация запуска движка
     */
    struct EngineConfig
    {
        std::string Title = "EOA Engine";
        uint32 Width = 1920;
        uint32 Height = 1080;
        bool Fullscreen = false;
        bool VSync = true;
        float TargetFPS = 60.0f;
        std::string StartingMap;
    };

    /**
     * @brief Главный класс движка (аналог UEngine в UE)
     * Управляет основным циклом, инициализацией подсистем и глобальным состоянием.
     */
    class EOA_API Engine
    {
    public:
        Engine();
        ~Engine();

        /**
         * @brief Инициализация движка
         * @param config Конфигурация запуска
         * @return true если успешно
         */
        bool Initialize(const EngineConfig& config);

        /**
         * @brief Запуск основного цикла
         */
        void Run();

        /**
         * @brief Остановка движка
         */
        void Exit();

        /**
         * @brief Получить единственный экземпляр движка
         */
        static Engine* Get() { return s_Instance; }

        // --- Доступ к подсистемам ---
        InputSystem* GetInputSystem() const { return m_InputSystem.get(); }
        eoa::World* GetCurrentWorld() const { return m_CurrentWorld.get(); }
        GameInstance* GetGameInstance() const { return m_GameInstance.get(); }

        // --- Управление миром ---
        /**
         * @brief Загрузить и активировать новый мир (уровень)
         */
        bool LoadWorld(const std::string& mapPath);

        /**
         * @brief Обновление логики мира
         */
        void TickWorld(float deltaTime);

        // --- Статистика ---
        float GetFPS() const { return m_FPS; }
        float GetFrameTime() const { return m_FrameTime; }

    private:
        void ProcessInput();
        void Render();

        static Engine* s_Instance;

        // Основные подсистемы
        std::unique_ptr<WindowAdapter> m_WindowAdapter;
        std::unique_ptr<Renderer> m_Renderer;
        std::unique_ptr<InputSystem> m_InputSystem;
        std::unique_ptr<GameInstance> m_GameInstance;
        
        // Текущий активный мир
        std::unique_ptr<eoa::World> m_CurrentWorld;

        // Состояние цикла
        bool m_Running = false;
        Timer m_Timer;
        float m_FPS = 0.0f;
        float m_FrameTime = 0.0f;
        EngineConfig m_Config;
    };

    // Глобальный макрос для доступа к движку (аналог GEngine)
    #define gEngine EOA::Engine::Get()

} // namespace EOA
