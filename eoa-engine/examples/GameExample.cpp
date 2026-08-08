// ============================================
// EOA ENGINE - Пример использования
// ============================================

#include "EoaEngine.h"
#include <iostream>

using namespace EOA;

// ============================================
// КАСТОМНЫЙ КОМПОНЕНТ
// ============================================

class RotatorComponent : public Component {
public:
    void Initialize() override {
        EOA_LOG_INFO("RotatorComponent initialized");
    }
    
    void Update(float deltaTime) override {
        auto* owner = GetOwner();
        if (!owner || !IsActive()) return;
        
        Vector3 rot = owner->GetRotation();
        rot.Y += 45.0f * deltaTime; // Вращение вокруг Y
        owner->SetRotation(rot);
    }
};

// ============================================
// КАСТОМНЫЙ АКТОР
// ============================================

class PlayerActor : public Actor {
public:
    void Initialize() {
        SetName("Player");
        AddComponent<RotatorComponent>();
        EOA_LOG_INFO("PlayerActor spawned");
    }
    
    void Update(float deltaTime) override {
        Actor::Update(deltaTime);
        
        // Пример управления
        if (IsKeyPressed(KeyCode::W)) {
            Vector3 pos = GetPosition();
            pos.Z += 5.0f * deltaTime;
            SetPosition(pos);
        }
        if (IsKeyPressed(KeyCode::S)) {
            Vector3 pos = GetPosition();
            pos.Z -= 5.0f * deltaTime;
            SetPosition(pos);
        }
    }
};

// ============================================
// GAME INSTANCE (в стиле UE: UGameInstance)
// ============================================

class MyGameInstance {
public:
    void Initialize() {
        Score = 0;
        CurrentLevel = 1;
        EOA_LOG_INFO("GameInstance initialized");
    }
    
    void AddScore(int points) {
        Score += points;
        EOA_LOG_INFO("Score: " + std::to_string(Score));
    }
    
    int GetScore() const { return Score; }
    
private:
    int Score = 0;
    int CurrentLevel = 1;
};

MyGameInstance* GameInst = nullptr;

// ============================================
// УРОВЕНЬ (в стиле UE: ULevel)
// ============================================

class MainLevel {
public:
    void Load() {
        EOA_LOG_INFO("Loading MainLevel...");
        
        // Спавн игрока
        Player = gWorld->SpawnActor<PlayerActor>();
        Player->SetPosition(Vector3(0, 0, 10));
        
        // Спавн врагов
        for (int i = 0; i < 5; i++) {
            auto* enemy = gWorld->SpawnActor<PlayerActor>();
            enemy->SetName("Enemy_" + std::to_string(i));
            enemy->SetPosition(Vector3((i - 2) * 3.0f, 0, 10));
        }
        
        EOA_LOG_INFO("MainLevel loaded with " + 
                     std::to_string(gWorld->GetActorCount()) + " actors");
    }
    
    void Unload() {
        EOA_LOG_INFO("Unloading MainLevel...");
        gWorld->Clear();
    }
    
    void Update(float deltaTime) {
        // Проверка условий победы/поражения
        if (gInput->IsKeyJustPressed(KeyCode::Escape)) {
            gEngine->Quit();
        }
        
        // Пример добавления очков
        if (gInput->IsKeyJustPressed(KeyCode::Space)) {
            GameInst->AddScore(100);
        }
    }
    
private:
    PlayerActor* Player = nullptr;
};

MainLevel* CurrentLevel = nullptr;

// ============================================
// КОНФИГУРАЦИЯ ПРИЛОЖЕНИЯ
// ============================================

class MyGameConfig {
public:
    static void Configure(EngineConfig& config) {
        config.Title = "My EOA Game";
        config.Width = 1280;
        config.Height = 720;
        config.Fullscreen = false;
        config.VSync = true;
        config.ContentPath = "assets/";
        config.EditorMode = false;
    }
};

// ============================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================

int main(int argc, char** argv) {
    // Настройка логгера
    Logger::GetInstance()->SetLogLevel(LogLevel::Info);
    Logger::GetInstance()->OpenLogFile("game.log");
    
    EOA_LOG_INFO("=== EOA Engine v" + std::string(GetVersionString()) + " ===");
    EOA_LOG_INFO("Starting game...");
    
    // Конфигурация
    EngineConfig config;
    MyGameConfig::Configure(config);
    
    // Запуск движка
    int result = Application::Run(config);
    
    Logger::GetInstance()->CloseLogFile();
    EOA_LOG_INFO("Game ended with code: " + std::to_string(result));
    
    return result;
}
