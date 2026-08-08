#include "log.h"
#include "core/eoa_application.h"
#include <chrono>

int main() {
    EOA_LOG("EOA Engine starting...");

    // Создаем и настраиваем конфигурацию движка
    EOA::EngineConfig config;
    config.Title = "EOA Engine - Editor";
    config.Width = 1920;
    config.Height = 1080;
    config.Fullscreen = false;
    config.VSync = true;
    config.TargetFPS = 60.0f;
    // config.StartingMap = "maps/level1.json"; // Раскомментировать для загрузки уровня

    // Создаем экземпляр движка
    EOA::Engine engine;
    
    // Инициализируем все подсистемы
    if (!engine.Initialize(config))
    {
        EOA_LOG_ERROR("Failed to initialize EOA Engine!");
        return 1;
    }

    EOA_LOG("Engine initialized successfully. Starting main loop...");
    
    // Запускаем главный цикл
    engine.Run();

    EOA_LOG("Shutdown clean.");
    return 0;
}
