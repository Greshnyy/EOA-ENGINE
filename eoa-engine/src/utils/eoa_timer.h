#pragma once

#include "core/eoa_core.h"
#include <chrono>

namespace EOA
{
    /**
     * @brief Простой таймер для измерения времени (аналог FPlatformTime в UE)
     */
    class Timer
    {
    public:
        Timer()
        {
            Reset();
        }

        /**
         * @brief Сброс таймера на текущий момент
         */
        void Reset()
        {
            m_Start = std::chrono::steady_clock::now();
        }

        /**
         * @brief Получить прошедшее время в секундах
         */
        float GetElapsedSeconds() const
        {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<float>(now - m_Start).count();
        }

        /**
         * @brief Получить прошедшее время в миллисекундах
         */
        double GetElapsedMilliseconds() const
        {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<double, std::milli>(now - m_Start).count();
        }

    private:
        std::chrono::steady_clock::time_point m_Start;
    };
}
