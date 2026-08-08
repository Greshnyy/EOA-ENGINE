#pragma once

#include "Core/Types.h"
#include <chrono>

namespace EOA {

// ============================================
// СИСТЕМА ВРЕМЕНИ (Time System)
// ============================================

class TimeSystem {
public:
    static TimeSystem* GetInstance() {
        static TimeSystem instance;
        return &instance;
    }
    
    // Дельта времени (секунды)
    float DeltaTime() const { return DeltaTimeSeconds; }
    
    // Общее время игры (секунды)
    float TotalTime() const { return TotalTimeSeconds; }
    
    // FPS
    uint32_t FPS() const { return CurrentFPS; }
    
    // Масштаб времени (для slow-mo / pause)
    void SetTimeScale(float scale) { TimeScale = scale; }
    float GetTimeScale() const { return TimeScale; }
    
    // Обновление (вызывать каждый кадр)
    void Update() {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = now - LastFrameTime;
        
        DeltaTimeSeconds = delta.count() * TimeScale;
        TotalTimeSeconds += DeltaTimeSeconds;
        
        // Расчет FPS
        FrameCount++;
        FPSTimer += delta.count();
        if (FPSTimer >= 1.0f) {
            CurrentFPS = FrameCount;
            FrameCount = 0;
            FPSTimer = 0.0f;
        }
        
        LastFrameTime = now;
    }
    
    // Пауза
    void Pause() { TimeScale = 0.0f; }
    void Resume() { TimeScale = 1.0f; }
    bool IsPaused() const { return TimeScale == 0.0f; }
    
private:
    TimeSystem() {
        LastFrameTime = std::chrono::high_resolution_clock::now();
    }
    
    std::chrono::time_point<std::chrono::high_resolution_clock> LastFrameTime;
    float DeltaTimeSeconds = 0.0f;
    float TotalTimeSeconds = 0.0f;
    float TimeScale = 1.0f;
    
    uint32_t CurrentFPS = 0;
    uint32_t FrameCount = 0;
    float FPSTimer = 0.0f;
};

#define EOA_TIME EOA::TimeSystem::GetInstance()

} // namespace EOA
