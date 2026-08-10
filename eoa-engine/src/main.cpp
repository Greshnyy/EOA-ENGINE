#include "core/Engine.h"

int main() {
    eoa::EngineConfig config;
    config.title = "EOA Engine - Editor";
    config.width = 1920;
    config.height = 1080;
    config.fullscreen = false;
    config.vsync = true;
    config.targetFPS = 60;
    config.fixedTimestep = true;
    config.fixedDeltaTime = 0.02;

    auto& engine = eoa::Engine::Get();
    if (!engine.Initialize(config)) {
        EOA_LOG_ERROR("Failed to initialize EOA Engine!");
        return 1;
    }

    EOA_LOG_INFO("Engine initialized successfully. Starting main loop...");
    engine.Run();
    return 0;
}
