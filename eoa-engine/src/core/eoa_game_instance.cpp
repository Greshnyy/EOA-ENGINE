#include "core/eoa_game_instance.h"
#include "log.h"

namespace EOA
{
    GameInstance::GameInstance()
    {
        LOG_INFO("GameInstance created");
    }

    GameInstance::~GameInstance()
    {
        LOG_INFO("GameInstance destroyed");
    }

    void GameInstance::Init()
    {
        LOG_INFO("GameInstance initialized");
        
        // Инициализация глобальных систем игры
        // - Менеджер сессий (для мультиплеера)
        // - Система достижений
        // - Глобальные настройки
        // - Инвентарь игрока
        
        m_PlayerCount = 1;
        m_PlayInEditor = false;
    }

    void GameInstance::Tick(float deltaTime)
    {
        // Обновление глобальных систем игры
        // Например:
        // - Проверка таймеров сессий
        // - Обновление статистики
        // - Обработка сетевых событий
    }

    void GameInstance::OnPreLoadWorld(const std::string& worldName)
    {
        LOG_INFO("Preparing to load world: {}", worldName);
        
        // Очистка данных предыдущего уровня, которые не должны сохраняться
        // Сохранение временного состояния при переходе между уровнями
    }

    void GameInstance::OnPostLoadWorld(const std::string& worldName)
    {
        LOG_INFO("World loaded: {}", worldName);
        m_CurrentMapName = worldName;
        
        // Инициализация систем после загрузки уровня
        // - Спавн игроков
        // - Настройка UI
        // - Запуск музыки уровня
    }

    void GameInstance::OnPreUnloadWorld(const std::string& worldName)
    {
        LOG_INFO("Preparing to unload world: {}", worldName);
        
        // Очистка ресурсов уровня
        // - Остановка звуков
        // - Удаление временных объектов
        // - Сохранение прогресса
    }

} // namespace EOA
