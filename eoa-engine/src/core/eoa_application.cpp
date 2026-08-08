#include "core/eoa_application.h"
#include "core/eoa_window_adapter.h"
#include "log.h"
#include <iostream>

namespace EOA
{
    Engine* Engine::s_Instance = nullptr;

    Engine::Engine()
    {
        s_Instance = this;
        LOG_INFO("EOA Engine instance created");
    }

    Engine::~Engine()
    {
        LOG_INFO("EOA Engine shutting down...");
        
        // Очистка в обратном порядке создания
        m_CurrentWorld.reset();
        m_GameInstance.reset();
        m_InputSystem.reset();
        m_Renderer.reset();
        m_WindowAdapter.reset();
        
        s_Instance = nullptr;
    }

    bool Engine::Initialize(const EngineConfig& config)
    {
        m_Config = config;
        LOG_INFO("Initializing EOA Engine: {}x{}, Title='{}'", 
                 config.Width, config.Height, config.Title);

        // 1. Создаем Input System первой (до окна)
        m_InputSystem = std::make_unique<InputSystem>();

        // 2. Создаем окно с адаптером для ввода
        m_WindowAdapter = std::make_unique<WindowAdapter>();
        m_WindowAdapter->Initialize(
            config.Width, 
            config.Height, 
            config.Title, 
            m_InputSystem.get()
        );

        // 3. Renderer (инициализируется после окна для получения Surface)
        // TODO: m_Renderer = std::make_unique<Renderer>();
        // if (!m_Renderer->Initialize(*m_WindowAdapter->GetWindow()))
        // {
        //     LOG_ERROR("Failed to initialize Renderer");
        //     return false;
        // }

        // 4. Инициализируем Input System с окном
        m_InputSystem->Initialize(*m_WindowAdapter->GetWindow());

        // 5. Создаем GameInstance
        m_GameInstance = std::make_unique<GameInstance>();
        m_GameInstance->Init();

        // 6. Если указан стартовый уровень, загружаем его
        if (!config.StartingMap.empty())
        {
            LoadWorld(config.StartingMap);
        }
        else
        {
            // Создаем пустой мир по умолчанию
            m_CurrentWorld = std::make_unique<World>("DefaultWorld");
        }

        LOG_INFO("All subsystems initialized successfully");
        return true;
    }

    void Engine::Run()
    {
        LOG_INFO("Starting engine main loop...");
        m_Running = true;
        m_Timer.Reset();

        while (m_Running)
        {
            // Расчет deltaTime
            float deltaTime = m_Timer.GetElapsedSeconds();
            m_Timer.Reset();

            m_FrameTime = deltaTime;
            m_FPS = 1.0f / (deltaTime > 0 ? deltaTime : 0.016f);

            // 1. Обработка событий окна
            if (m_WindowAdapter)
            {
                m_WindowAdapter->PollEvents();
                
                if (m_WindowAdapter->ShouldClose())
                {
                    Exit();
                    continue;
                }
            }

            // 2. Обновление ввода
            if (m_InputSystem)
            {
                m_InputSystem->Update();
            }

            // 3. Обновление логики
            TickWorld(deltaTime);
            if (m_GameInstance)
            {
                m_GameInstance->Tick(deltaTime);
            }

            // 4. Рендеринг
            Render();
        }
    }

    void Engine::Exit()
    {
        LOG_INFO("Engine exit requested");
        m_Running = false;
    }

    bool Engine::LoadWorld(const std::string& mapPath)
    {
        LOG_INFO("Loading world: {}", mapPath);
        
        // Сохраняем текущий мир (если есть) для очистки
        auto oldWorld = std::move(m_CurrentWorld);
        
        // Создаем новый мир
        m_CurrentWorld = std::make_unique<World>(mapPath);
        
        // Уведомляем GameInstance
        if (m_GameInstance)
        {
            m_GameInstance->OnPostLoadWorld(mapPath);
        }
        
        LOG_INFO("World '{}' loaded successfully", mapPath);
        return true;
    }

    void Engine::TickWorld(float deltaTime)
    {
        if (m_CurrentWorld)
        {
            m_CurrentWorld->Tick(deltaTime);
        }
    }

    void Engine::ProcessInput()
    {
        if (m_InputSystem)
        {
            // Глобальные хоткеи движка
            if (m_InputSystem->IsKeyPressed(KeyCode::Escape))
            {
                // В редакторе: выход из режима Play
                // В игре: пауза или меню
                LOG_DEBUG("Escape key pressed");
            }
            
            if (m_InputSystem->IsKeyPressed(KeyCode::F11))
            {
                // Toggle fullscreen
                LOG_DEBUG("F11 pressed - toggle fullscreen");
            }
        }
    }

    void Engine::Render()
    {
        // TODO: Реализовать рендеринг
        // if (!m_Renderer || !m_CurrentWorld)
        //     return;

        // Начало кадра
        // m_Renderer->BeginFrame();

        // Рендер мира
        // m_CurrentWorld->Render(*m_Renderer);

        // Рендер UI (ImGui)
        // TODO: m_EditorUI->Render();

        // Конец кадра
        // m_Renderer->EndFrame();
    }

} // namespace EOA
