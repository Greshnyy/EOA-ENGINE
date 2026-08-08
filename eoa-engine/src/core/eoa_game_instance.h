#pragma once

#include "core/eoa_core.h"
#include "reflection/eoa_reflection.h"

namespace EOA
{
    /**
     * @brief GameInstance - аналог UGameInstance в UE
     * Живет на протяжении всего времени работы приложения, 
     * переживает загрузку/выгрузку уровней.
     * Используется для хранения глобального состояния игры,
     * менеджеров сессий, инвентаря, достижений и т.д.
     */
    class EOA_API GameInstance : public Reflectable
    {
        EOA_CLASS(GameInstance, "Core", "Экземпляр игры, живущий всё время работы приложения")

    public:
        GameInstance();
        virtual ~GameInstance();

        /**
         * @brief Инициализация при старте движка
         */
        virtual void Init();

        /**
         * @brief Обновление каждый кадр
         * @param deltaTime Время между кадрами
         */
        virtual void Tick(float deltaTime);

        /**
         * @brief Вызывается перед загрузкой нового мира
         */
        virtual void OnPreLoadWorld(const std::string& worldName);

        /**
         * @brief Вызывается после загрузки мира
         */
        virtual void OnPostLoadWorld(const std::string& worldName);

        /**
         * @brief Вызывается перед уничтожением мира
         */
        virtual void OnPreUnloadWorld(const std::string& worldName);

        // --- Глобальное состояние игры ---
        
        /**
         * @brief Получить имя текущей карты
         */
        const std::string& GetCurrentMapName() const { return m_CurrentMapName; }

        /**
         * @brief Установить имя текущей карты
         */
        void SetCurrentMapName(const std::string& name) { m_CurrentMapName = name; }

        /**
         * @brief Получить количество игроков в игре
         */
        int32 GetPlayerCount() const { return m_PlayerCount; }

        /**
         * @brief Установить количество игроков
         */
        void SetPlayerCount(int32 count) { m_PlayerCount = count; }

        /**
         * @brief Проверка, запущена ли игра в режиме редактора
         */
        bool IsPlayInEditor() const { return m_PlayInEditor; }

        /**
         * @brief Установить режим Play In Editor
         */
        void SetPlayInEditor(bool value) { m_PlayInEditor = value; }

    protected:
        // Имя текущей загруженной карты
        std::string m_CurrentMapName;

        // Количество игроков (для мультиплеера)
        int32 m_PlayerCount = 1;

        // Флаг режима игры из редактора
        bool m_PlayInEditor = false;

        // Дополнительные данные для сохранения между уровнями
        // (инвентарь, прогресс, настройки и т.д.)
        std::unordered_map<std::string, std::any> m_PersistentData;
    };

} // namespace EOA
